#pragma once

#include "benchmark.hpp"

#include <list.hpp>
#include <thread>

template <class Reclaimer>
struct list_benchmark : benchmark_with_reclaimer<Reclaimer>
{
  virtual void setup(const boost::program_options::variables_map& vm) override;
  virtual void run(thread_local_data& data) override;
  virtual std::string get_params() override;

private:
  unsigned number_of_elements;
  unsigned search_operations;
  unsigned modify_operations;

  emr::list<unsigned, Reclaimer> list;

  const size_t bits_for_operation_count = 10;

  void populate_list()
  {
    // we are population the list in a separate thread to avoid having the main thread
    // in the global threadlists of the reclaimers.
    // this is especially important in case of QSBR since the main thread never explicitly
    // goes through a quiescent state.
    std::thread initializer([this]()
    {
      typename Reclaimer::region_guard rg{};
      for (unsigned i = 0, j = 0; i < number_of_elements; ++i, j+= 2)
        list.insert(j);
    });
    initializer.join();
  }
};

template <class Reclaimer>
void list_benchmark<Reclaimer>::setup(const boost::program_options::variables_map& vm)
{
  number_of_elements = vm["elements"].as<unsigned>();

  auto operations = (1 << bits_for_operation_count);
  if (vm.count("modify-fraction"))
  {
    auto fraction = vm["modify-fraction"].as<double>();
    if (fraction < 0.0 || fraction > 1.0)
      throw std::runtime_error("modify-fraction must be >= 0 and <= 1");

    modify_operations = static_cast<unsigned>(operations * fraction);
  }
  else
    modify_operations = operations / 2;

  search_operations = operations - modify_operations;

  populate_list();
}

template <class Reclaimer>
void list_benchmark<Reclaimer>::run(thread_local_data& data)
{
  const size_t n = 100;

  const auto operations = 1 << bits_for_operation_count;
  const unsigned range = operations + 1;
  const unsigned mask = operations - 1;
  const unsigned number_of_keys = std::max(1u, number_of_elements * 2);

  typename Reclaimer::region_guard region_guard{};
  for (size_t i = 0; i < n; i++)
  {
    auto r = data.randomizer();
    auto insertOp = r & 1;
    r >>= 1;
    auto action = ((r & mask) % range);
    r >>= bits_for_operation_count;
    auto key = r % number_of_keys;

    if (action <= search_operations)
      list.search(key);
    else if (insertOp)
      list.insert(key);
    else
      list.remove(key);
  }

  // Record another n operations.
  data.number_of_operations += n;
}

template <class Reclaimer>
std::string list_benchmark<Reclaimer>::get_params()
{
  return "elements: " + std::to_string(number_of_elements) +
         "; modify-fraction: " +
         std::to_string(modify_operations / static_cast<double>(search_operations + modify_operations));
}
