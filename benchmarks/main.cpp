#include "guard_ptr_benchmark.hpp"
#include "list_benchmark.hpp"
#include "queue_benchmark.hpp"
#include "hash_map_benchmark.hpp"
#include "test_execution.hpp"
#include "output_formatter.hpp"
#include "console_output_formatter.hpp"
#include "csv_output_formatter.hpp"

#include <emr/dummy.hpp>
#include <emr/lock_free_ref_count.hpp>
#include <emr/hazard_pointer.hpp>
#include <emr/epoch_based.hpp>
#include <emr/new_epoch_based.hpp>
#include <emr/quiescent_state_based.hpp>
#include <emr/stamp_it.hpp>

#include <boost/program_options.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

template <template <class> class Benchmark, class Reclaimer>
struct benchmark_builder
{
  std::shared_ptr<benchmark> operator()() { return std::make_shared<Benchmark<Reclaimer>>(); }
};

// map from reclamation scheme to builders of instations of specific bechmarks with this reclaimer.
using benchmark_variations = std::unordered_map<
  std::string,
  std::function<std::shared_ptr<benchmark>()>
>;

template <template <class> class Benchmark>
auto make_benchmark_variations()
{
  return benchmark_variations
  {
    { "none", benchmark_builder<Benchmark, emr::dummy>() },
    { "LFRC", benchmark_builder<Benchmark, emr::lock_free_ref_count<false, 0>>() },
    { "LFRC-padded", benchmark_builder<Benchmark, emr::lock_free_ref_count<true, 0>>() },
    { "LFRC-unpadded-10", benchmark_builder<Benchmark, emr::lock_free_ref_count<false, 10>>() },
    { "LFRC-unpadded-20", benchmark_builder<Benchmark, emr::lock_free_ref_count<false, 20>>() },
    { "LFRC-unpadded-100", benchmark_builder<Benchmark, emr::lock_free_ref_count<false, 100>>() },
    { "LFRC-padded-10", benchmark_builder<Benchmark, emr::lock_free_ref_count<true, 10>>() },
    { "LFRC-padded-20", benchmark_builder<Benchmark, emr::lock_free_ref_count<true, 20>>() },
    { "LFRC-padded-100", benchmark_builder<Benchmark, emr::lock_free_ref_count<true, 100>>() },
    { "static-HPBR", benchmark_builder<Benchmark, emr::hazard_pointer<emr::static_hazard_pointer_policy<>>>() },
    { "dynamic-HPBR", benchmark_builder<Benchmark, emr::hazard_pointer<emr::dynamic_hazard_pointer_policy<>>>() },
    { "dynamic-HPBR-strict", benchmark_builder<Benchmark, emr::hazard_pointer<emr::dynamic_hazard_pointer_policy<2,1,0>>>() },
    { "EBR",  benchmark_builder<Benchmark, emr::epoch_based<100>>() },
    { "NEBR", benchmark_builder<Benchmark, emr::new_epoch_based<100>>() },
    { "QSBR", benchmark_builder<Benchmark, emr::quiescent_state_based>() },
    { "stamp", benchmark_builder<Benchmark, emr::stamp_it>() }
  };
}

namespace po = boost::program_options;

class runner
{
  unsigned number_of_threads = 0;
  unsigned number_of_trials = 0;
  unsigned memory_samples = 0;
  unsigned runtime = 0;
  unsigned elements;
  std::string benchmark_name;
  std::string reclaimer_name;
  std::shared_ptr<::output_formatter> output_formatter;

  po::options_description desc;
  po::variables_map vm;

  void parse_arguments(int argc, char* argv[]);
  std::shared_ptr<benchmark> get_benchmark();
public:
  runner(int argc, char* argv[]);
  void run();
};

runner::runner(int argc, char* argv[])
{
  parse_arguments(argc, argv);
}

void runner::parse_arguments(int argc, char* argv[])
{
  desc.add_options()
    (
      "benchmark",
      po::value<std::string>(&benchmark_name)->required(),
      "the benchmark to run"
    )
    (
      "reclaimer",
      po::value<std::string>(&reclaimer_name)->required(),
      "the reclamation scheme to use"
    )
    (
      "runtime",
      po::value<unsigned>(&runtime)->default_value(10000),
      "runtime of each trial in milliseconds"
    )
    (
      "modify-fraction",
      po::value<double>(),
      "fraction of modify operations in percent (between 0.0 and 1.0) - only for list benchmark"
    )
    (
      "elements",
      po::value<unsigned>(&elements)->default_value(10),
      "the initial number of elements in the list/queue"
    )
    (
      "threads",
      po::value<unsigned>(&number_of_threads)->default_value(4),
      "the number of threads"
    )
    (
      "trials",
      po::value<unsigned>(&number_of_trials)->default_value(8),
      "the number of trials to perform"
    )
    (
      "memory-samples",
      po::value<unsigned>(&memory_samples)->default_value(0),
      "the number of memory usage samples per trial"
    )
    (
      "csv",
      po::value<std::string>(),
      "path to a CSV file for result output"
    )
    ("help", "output this message")
  ;

  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("csv") > 0)
    output_formatter = std::make_shared<csv_output_formatter>(vm["csv"].as<std::string>());
  else
    output_formatter = std::make_shared<console_output_formatter>();
}

void runner::run()
{
  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return;
  }

  po::notify(vm);

  auto benchmark = get_benchmark();
  auto starttime = std::chrono::system_clock::now();
  for (unsigned i = 0; i < number_of_trials; ++i)
  {
    try
    {
      test_execution execution(number_of_threads, i, runtime, starttime, memory_samples, benchmark);
      execution.run();
#ifdef TRACK_ALLOCATIONS
      benchmark->allocation_tracker().collapse_counters();
#endif

      data_record record;
      record.add("trial", std::to_string(i + 1));
      record.add("benchmark", benchmark_name);
      record.add("reclaimer", reclaimer_name);
      record.add("reclaimer-type", benchmark->reclaimer_type().name());
      record.add("params", benchmark->get_params());
      execution.get_data(record);
      this->output_formatter->write(record);
    }
    catch (std::exception& e)
    {
        std::cout << "Trial " << i << " failed: " << e.what() << std::endl;
    }
  }
}

std::shared_ptr<benchmark> runner::get_benchmark()
{
  std::unordered_map<std::string, benchmark_variations> benchmarks =
  {
    { "list",  make_benchmark_variations<list_benchmark>() },
    { "queue", make_benchmark_variations<queue_benchmark>() },
    { "hash_map", make_benchmark_variations<hash_map_benchmark>() },
    { "guard_ptr", make_benchmark_variations<guard_ptr_benchmark>() }
  };

  auto benchmark_name_it = benchmarks.find(benchmark_name);
  if (benchmark_name_it == benchmarks.end())
    throw std::runtime_error("Invalid benchmark - " + benchmark_name);

  auto& reclaimer_map=  benchmark_name_it->second;
  auto reclaimer_name_it = reclaimer_map.find(reclaimer_name);
  if (reclaimer_name_it == reclaimer_map.end())
    throw std::runtime_error("Invalid reclaimer - " + reclaimer_name);

  auto benchmark = reclaimer_name_it->second();
  benchmark->setup(vm);
  return benchmark;
}

int main (int argc, char* argv[])
{
#if !defined(NDEBUG)
  std::cout
    << "==============================\n"
    << "  This is a __DEBUG__ build!  \n"
    << "=============================="
    << std::endl;
#endif
  try
  {
    runner runner(argc, argv);
    runner.run();
  }
  catch (const std::exception& e)
  {
    std::cout << "ERROR: " << e.what() << std::endl;
  }
}
