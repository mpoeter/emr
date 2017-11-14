#pragma once

#include "benchmark.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

class test_execution
{
public:
  test_execution(unsigned number_of_threads, unsigned trial, unsigned runtime,
                 std::chrono::system_clock::time_point starttime, unsigned memory_samples,
                 std::shared_ptr<benchmark> benchmark);
  void run();
  void get_data(data_record& record);

private:
  void wait_until_all_threads_are_running();
  void wait_until_all_threads_are_finished();
  void start_benchmarks();
  double nanoseconds_per_operation();

  enum class test_state
  {
    starting_threads,
    not_started,
    running,
    over
  };

  class alignas(64) testing_thread
  {
  public:
    void* operator new(size_t sz);
    void operator delete(void* p);

    testing_thread(
      const std::atomic<test_state>& test_state,
      std::shared_ptr<benchmark> benchmark,
      unsigned seed,
      size_t processorGroup);
    ~testing_thread();

    void wait_until_running() const;
    void wait_until_finished() const;
    double nanoseconds_per_operation() const;
  private:
    void thread_func();
    void wait_until_all_threads_are_started();
    void wait_until_running_state_is(bool state) const;
    void wait_until_benchmark_starts();
    const std::atomic<test_execution::test_state>& test_state;
    std::atomic<bool> is_running;
    std::shared_ptr<::benchmark> benchmark;
    thread_local_data data;
    std::thread thread;
  };

  struct memory_sample
  {
    size_t rss;
#ifdef TRACK_ALLOCATIONS
    size_t allocated_instances;
    size_t reclaimed_instances;
#endif
    std::chrono::milliseconds runtime;
  };

  unsigned runtime;
  std::vector<std::unique_ptr<testing_thread>> threads;
  std::chrono::system_clock::time_point starttime;
  unsigned number_of_memory_samples;
  std::vector<memory_sample> memory_samples;
  std::atomic<test_state> state;
  std::shared_ptr<::benchmark> benchmark;
};