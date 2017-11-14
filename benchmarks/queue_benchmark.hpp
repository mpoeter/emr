#pragma once

#include "benchmark.hpp"

#include <queue.hpp>
#include <thread>

template <class Reclaimer>
struct queue_benchmark : benchmark_with_reclaimer<Reclaimer>
{
  virtual void setup(const boost::program_options::variables_map& vm) override;
  virtual void run(thread_local_data& data) override;
  virtual std::string get_params() override;

private:
  unsigned number_of_elements;
  emr::queue<int, Reclaimer> queue;
};

template <class Reclaimer>
void queue_benchmark<Reclaimer>::setup(const boost::program_options::variables_map& vm)
{
  if (vm.count("elements"))
    number_of_elements = vm["elements"].as<unsigned>();

  // we are population the queue in a separate thread to avoid having the main thread
  // in the global threadlists of the reclaimers.
  // this is especially important in case of QSBR since the main thread never explicitly
  // goes through a quiescent state.
  std::thread initializer([this]()
  {
    typename Reclaimer::region_guard rg{};
    for (unsigned i = 0, j = 0; i < number_of_elements; ++i, j += 2)
      queue.enqueue(j);
  });
  initializer.join();
}

template <class Reclaimer>
void queue_benchmark<Reclaimer>::run(thread_local_data& data)
{
  const unsigned n = 100;
  const auto number_of_keys = std::max(1u, number_of_elements * 2);

  typename Reclaimer::region_guard region_guard{};
  for (unsigned i = 0; i < n; ++i)
  {
    auto r = data.randomizer();
    auto action = r & 1;
    auto key = (r >> 1) % number_of_keys;

    if (action)
      queue.enqueue(key);
    else
    {
      int value;
      queue.try_dequeue(value);
    }
  }

  // Record another n operations.
  data.number_of_operations += n;
}

template <class Reclaimer>
std::string queue_benchmark<Reclaimer>::get_params()
{
  return "elements: " + std::to_string(number_of_elements);
}
