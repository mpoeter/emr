#pragma once

#include "benchmark.hpp"

#include <emr/acquire_guard.hpp>

#include <thread>

template <class Reclaimer>
struct guard_ptr_benchmark : benchmark_with_reclaimer<Reclaimer>
{
  virtual void setup(const boost::program_options::variables_map& vm) override;
  virtual void run(thread_local_data& data) override;

private:
  struct node : Reclaimer::template enable_concurrent_ptr<node, 1> {};
  using concurrent_ptr = typename Reclaimer::template concurrent_ptr<node, 1>;
  concurrent_ptr obj;
};

template <class Reclaimer>
void guard_ptr_benchmark<Reclaimer>::setup(const boost::program_options::variables_map& vm)
{
  obj.store(new node(), std::memory_order_relaxed);
}

template <class Reclaimer>
void guard_ptr_benchmark<Reclaimer>::run(thread_local_data& data)
{
  const size_t n = 100;

  for (size_t i = 0; i < n; i++)
  {
    auto guard = emr::acquire_guard(obj, std::memory_order_relaxed);
  }

  // Record another n operations.
  data.number_of_operations += n;
}

