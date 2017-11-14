#pragma once

#include "output_formatter.hpp"

#include <emr/stamp_it.hpp>

#include <boost/program_options/variables_map.hpp>

#include <chrono>
#include <random>
#include <emr/detail/allocation_tracker.hpp>

struct thread_local_data
{
  std::mt19937 randomizer;
  size_t number_of_operations = 0;
  std::chrono::duration<double, std::nano> runtime;
};

struct benchmark
{
  virtual ~benchmark() {}
  virtual void setup(const boost::program_options::variables_map& vm) = 0;
  virtual void run(thread_local_data& data) = 0;
  virtual const std::type_info& reclaimer_type() const = 0;
  virtual std::string get_params() { return std::string(); }
  virtual void get_data(data_record& record) {}

#ifdef TRACK_ALLOCATIONS
  virtual emr::detail::allocation_tracker& allocation_tracker() = 0;
#endif
};

template <typename R>
inline void add_performance_counters(data_record& record) {}

template <class Reclaimer>
struct benchmark_with_reclaimer : benchmark
{
  virtual const std::type_info& reclaimer_type() const override
  {
    return typeid(Reclaimer);
  }

  virtual void get_data(data_record& record)
  {
    add_performance_counters<Reclaimer>(record);
  }

#ifdef TRACK_ALLOCATIONS
  virtual emr::detail::allocation_tracker& allocation_tracker()
  {
    return Reclaimer::allocation_tracker;
  }
#endif
};

#ifdef WITH_PERF_COUNTER
template <>
inline void add_performance_counters<emr::stamp_it>(data_record& record)
{
  auto cnt = emr::stamp_it::get_performance_counters();
  record.add("push_iterations", std::to_string(cnt.push_iterations / (double)cnt.push_calls));
  record.add("remove_next_iterations", std::to_string(cnt.remove_next_iterations / (double)cnt.remove_calls));
  record.add("remove_prev_iterations", std::to_string(cnt.remove_prev_iterations / (double)cnt.remove_calls));
}
#endif
