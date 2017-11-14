#pragma once

#include "benchmark.hpp"

#include <hash_map.hpp>
#include <queue.hpp>

#include <atomic>
#include <vector>

template <class Reclaimer>
struct hash_map_benchmark : benchmark_with_reclaimer<Reclaimer>
{
  virtual void setup(const boost::program_options::variables_map& vm) override;
  virtual void run(thread_local_data& data) override;
  virtual std::string get_params() override;

private:
  using Key = size_t;
  using Value = std::unique_ptr<double[]>;

  static const size_t BlockSize = 1024;
  static const size_t Buckets = 2048;
  static const size_t Blocks = 1000;
  static const size_t MaxCache = 10000;
  static const size_t DifferentBlockResults = 3 * MaxCache / Blocks;

  using hash_map = emr::hash_map<Key, Value, Reclaimer, Buckets>;

  Value calc_block(size_t idx);
  void limit_cache_size();

  std::atomic<size_t> number_of_cached_entries;
  std::atomic<size_t> cache_hits;
  std::atomic<size_t> cache_queries;
  unsigned threads;
  hash_map cache;
  emr::queue<Key, Reclaimer> keys;
};

template <class Reclaimer>
void hash_map_benchmark<Reclaimer>::setup(const boost::program_options::variables_map& vm)
{
  Key key;
  while (keys.try_dequeue(key))
    cache.remove(key);

  cache_hits.store(0, std::memory_order_relaxed);
  cache_queries.store(0, std::memory_order_relaxed);
  number_of_cached_entries.store(0, std::memory_order_relaxed);
}

template <class Reclaimer>
auto hash_map_benchmark<Reclaimer>::calc_block(size_t idx) -> Value
{
  Value result(new double[BlockSize]);
  // to simulate the cost of calculating a block we simply
  // perfom a bunch of meaningless but expensive computations
  for (size_t i = 0; i < BlockSize; ++i)
    result[i] = sqrt(sin(static_cast<float>(i)) * cos(static_cast<float>(i)));

  return result;
}

template <class Reclaimer>
void hash_map_benchmark<Reclaimer>::run(thread_local_data& data)
{
  size_t added_entries = 0;
  size_t local_cache_hits = 0;

  auto get_or_calc_block = [this, &data, &added_entries](size_t idx)
  {
    auto r = data.randomizer() % DifferentBlockResults;
    auto key = (r << 32) | idx;

    auto result = cache.search(key);
    if (!result)
    {
      auto value = calc_block(r);
      if (cache.insert(key, std::move(value), result))
      {
        keys.enqueue(key);
        ++added_entries;
      }
    }
    return result;
  };

  {
    std::vector<typename hash_map::guard_ptr> cache_entries(Blocks);

    typename Reclaimer::region_guard region_guard{};
    added_entries = 0;

    for (size_t block = 0; block < Blocks; ++block)
      cache_entries[block] = get_or_calc_block(block);

    number_of_cached_entries.fetch_add(added_entries, std::memory_order_relaxed);
    local_cache_hits += Blocks - added_entries;

    Value total_result(new double[BlockSize]);
    memset(total_result.get(), 0, sizeof(double) * BlockSize);
    for (auto& block : cache_entries)
    {
      auto block_result = block->value.get();
      for (size_t i = 0; i < BlockSize; ++i)
        total_result[i] += block_result[i];
      block.reset();
    }
  }
  {
    typename Reclaimer::region_guard region_guard{};
    limit_cache_size();
  }

  cache_hits.fetch_add(local_cache_hits, std::memory_order_relaxed);
  cache_queries.fetch_add(Blocks, std::memory_order_relaxed);
  data.number_of_operations += 1;
}

template <class Reclaimer>
void hash_map_benchmark<Reclaimer>::limit_cache_size()
{
  auto cache_size = number_of_cached_entries.load(std::memory_order_relaxed);
  while (cache_size > MaxCache)
  {
    Key key;
    if (!keys.try_dequeue(key))
      return;

    if (cache.remove(key))
      cache_size = --number_of_cached_entries;
    else
      assert(false); // this should never happen as every key exists only once in the queue
  }
}

template <class Reclaimer>
std::string hash_map_benchmark<Reclaimer>::get_params()
{
  return ""; //"cache-hit-rate: " + std::to_string(cache_hits.load() / (double)cache_queries.load());
}
