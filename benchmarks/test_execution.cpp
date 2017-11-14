#include "test_execution.hpp"
#include "process_memory.hpp"

#include <iostream>

test_execution::test_execution(
    unsigned number_of_threads,
    unsigned trial,
    unsigned runtime,
    std::chrono::system_clock::time_point starttime,
    unsigned memory_samples,
    std::shared_ptr<::benchmark> benchmark) :
  runtime(runtime),
  threads(),
  starttime(starttime),
  number_of_memory_samples(memory_samples),
  memory_samples(),
  state(test_state::starting_threads),
  benchmark(benchmark)
{
  this->memory_samples.reserve(memory_samples + 2);
#ifdef WIN32
  WORD processorGroups = GetActiveProcessorGroupCount();
#else
  size_t processorGroups = 1;
#endif

  threads.reserve(number_of_threads);
  for (unsigned i = 0; i < number_of_threads; ++i)
    threads.push_back(std::make_unique<testing_thread>(
      state, benchmark, std::mt19937::default_seed + i * (1 + trial), i % processorGroups));
}

void test_execution::run()
{
  state = test_state::not_started;
  wait_until_all_threads_are_running();

  auto samples = number_of_memory_samples;
  auto record_sample = [this]()
  {
    auto runtime = std::chrono::system_clock::now() - starttime;
    memory_sample sample;
    sample.rss = getCurrentRSS();
    sample.runtime = std::chrono::duration_cast<std::chrono::milliseconds>(runtime);
#ifdef TRACK_ALLOCATIONS
    auto counters = benchmark->allocation_tracker().get_counters();
    sample.allocated_instances = counters.first;
    sample.reclaimed_instances = counters.second;
#endif
    memory_samples.push_back(sample);
  };

  if (samples > 0)
    record_sample();

  start_benchmarks();

  // sleep for the specified time while threads are running (and maybe take some memory samples).
  if (samples > 0)
  {
    auto sleeptime = std::chrono::milliseconds(runtime / (samples + 1));
    for (size_t i = 0; i < samples; ++i)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleeptime));
      record_sample();
    }
  }
  else
    std::this_thread::sleep_for(std::chrono::milliseconds(runtime));


  // signal test end
  state = test_state::over;

  wait_until_all_threads_are_finished();
  if (samples > 0)
    record_sample();
}

void test_execution::wait_until_all_threads_are_running()
{
  for (auto& thread : threads)
    thread->wait_until_running();
}

void test_execution::start_benchmarks()
{
  state.store(test_state::running);
}

void test_execution::wait_until_all_threads_are_finished()
{
  for (auto& thread : threads)
    thread->wait_until_finished();
}

void test_execution::get_data(data_record& record)
{
  record.add("threads", std::to_string(threads.size()));
  record.add("runtime", std::to_string(runtime));
  record.add("ns/op", std::to_string(nanoseconds_per_operation()));
  for (size_t i = 0; i < memory_samples.size(); ++i)
  {
    auto& sample = memory_samples[i];
    record.add("memory_" + std::to_string(i), std::to_string(sample.rss));
    record.add("runtime_" + std::to_string(i), std::to_string(sample.runtime.count()));
#ifdef TRACK_ALLOCATIONS
    record.add("allocated_instances_" + std::to_string(i), std::to_string(sample.allocated_instances));
    record.add("reclaimed_instances_" + std::to_string(i), std::to_string(sample.reclaimed_instances));
#endif
  }
  benchmark->get_data(record);
}

double test_execution::nanoseconds_per_operation()
{
  double ns_per_op = 0;
  for (auto& thread : threads)
    ns_per_op += thread->nanoseconds_per_operation();

  return ns_per_op / threads.size();
}

void* test_execution::testing_thread::operator new(size_t sz)
{
  return boost::alignment::aligned_alloc(std::alignment_of<testing_thread>(), sz);
}

void test_execution::testing_thread::operator delete(void* p)
{
  boost::alignment::aligned_free(p);
}

test_execution::testing_thread::testing_thread(
    const std::atomic<test_execution::test_state>& test_state,
    std::shared_ptr<::benchmark> benchmark,
    unsigned seed,
    size_t processorGroup) :
  test_state(test_state),
  is_running(false),
  benchmark(std::move(benchmark)),
  thread(&testing_thread::thread_func, this)
{
  auto time = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  data.randomizer.seed(seed);

#ifdef WIN32
  if (GetActiveProcessorGroupCount() > 1)
  {
    GROUP_AFFINITY affinity{};
    affinity.Group = static_cast<WORD>(processorGroup);
    affinity.Mask = (1ull << 40) - 1;//(KAFFINITY)-1;
    GROUP_AFFINITY oldAffinity{};
    if (!SetThreadGroupAffinity(GetCurrentThread(), &affinity, &oldAffinity))
      std::cout << "SetThreadGroupAffinity failed - last error: " << GetLastError() << std::endl;
  }
#else
  (void)processorGroup;
#endif
}

test_execution::testing_thread::~testing_thread()
{
  if (thread.joinable())
    thread.join();
}

void test_execution::testing_thread::wait_until_running() const
{
  wait_until_running_state_is(true);
}

void test_execution::testing_thread::wait_until_finished() const
{
  wait_until_running_state_is(false);
}

void test_execution::testing_thread::wait_until_running_state_is(bool state) const
{
  while (is_running.load(std::memory_order_relaxed) != state)
    ;
}

double test_execution::testing_thread::nanoseconds_per_operation() const
{
  assert(is_running == false);
  return data.runtime.count() / data.number_of_operations;
}

void test_execution::testing_thread::thread_func()
{
  wait_until_all_threads_are_started();

  is_running.store(true);

  try
  {
    wait_until_benchmark_starts();

    auto start = std::chrono::high_resolution_clock::now();

    while (test_state.load(std::memory_order_relaxed) == test_execution::test_state::running)
      benchmark->run(data);

    data.runtime = std::chrono::high_resolution_clock::now() - start;
  }
  catch (std::exception& e)
  {
    std::cout << "Thread " << std::this_thread::get_id() << " failed: " << e.what() << std::endl;
  }

  is_running.store(false);
}

void test_execution::testing_thread::wait_until_all_threads_are_started()
{
  while (test_state.load(std::memory_order_relaxed) == test_execution::test_state::starting_threads)
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void test_execution::testing_thread::wait_until_benchmark_starts()
{
  while (test_state.load(std::memory_order_relaxed) == test_execution::test_state::not_started)
    ;
}
