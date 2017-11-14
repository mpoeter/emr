#include <emr/lock_free_ref_count.hpp>
#include <emr/hazard_pointer.hpp>
#include <emr/epoch_based.hpp>
#include <emr/new_epoch_based.hpp>
#include <emr/quiescent_state_based.hpp>
#include <emr/stamp_it.hpp>
#include <queue.hpp>

#include <gtest/gtest.h>

#include <vector>
#include <thread>

namespace {

template <typename Reclaimer>
struct Queue : testing::Test {};

using Reclaimers = ::testing::Types<
    emr::lock_free_ref_count<>,
    emr::hazard_pointer<emr::static_hazard_pointer_policy<2>>,
    emr::epoch_based<10>,
    emr::new_epoch_based<10>,
    emr::quiescent_state_based,
    emr::stamp_it
  >;
TYPED_TEST_CASE(Queue, Reclaimers);

TYPED_TEST(Queue, enqueue_try_deque_returns_enqueued_element)
{
  emr::queue<int, TypeParam> queue;
  queue.enqueue(42);
  int elem;
  queue.try_dequeue(elem);
  EXPECT_EQ(42, elem);
}

TYPED_TEST(Queue, enqueue_two_items_deque_them_in_FIFO_order)
{
  emr::queue<int, TypeParam> queue;
  queue.enqueue(42);
  queue.enqueue(43);
  int elem1, elem2;
  queue.try_dequeue(elem1);
  queue.try_dequeue(elem2);
  EXPECT_EQ(42, elem1);
  EXPECT_EQ(43, elem2);
}

TYPED_TEST(Queue, parallel_usage)
{
  using Reclaimer = TypeParam;
  emr::queue<int, TypeParam> queue;

  std::vector<std::thread> threads;
  for (int i = 0; i < 12; ++i)
  {
    threads.push_back(std::thread([i, &queue]
    {
    #ifdef _DEBUG
      const int MaxIterations = 1000;
    #else
      const int MaxIterations = 10000;
    #endif
      for (int j = 0; j < MaxIterations; ++j)
      {
        typename Reclaimer::region_guard critical_region{};
        queue.enqueue(i);
        int v;
        EXPECT_TRUE(queue.try_dequeue(v));
      }
    }));
  }

  for (auto& thread : threads)
    thread.join();
}
}