#include <emr/lock_free_ref_count.hpp>
#include <emr/hazard_pointer.hpp>
#include <emr/epoch_based.hpp>
#include <emr/new_epoch_based.hpp>
#include <emr/quiescent_state_based.hpp>
#include <emr/stamp_it.hpp>
#include <list.hpp>

#include <gtest/gtest.h>

#include <vector>
#include <thread>

namespace {

template <typename Reclaimer>
struct List : testing::Test {};

using Reclaimers = ::testing::Types<
    emr::lock_free_ref_count<>,
    emr::hazard_pointer<emr::static_hazard_pointer_policy<3>>,
    emr::epoch_based<10>,
    emr::new_epoch_based<10>,
    emr::quiescent_state_based,
    emr::stamp_it
  >;
TYPED_TEST_CASE(List, Reclaimers);

TYPED_TEST(List, insert_same_element_twice_second_time_fails)
{
  emr::list<int, TypeParam> list;
  EXPECT_TRUE(list.insert(42));
  EXPECT_FALSE(list.insert(42));
}

TYPED_TEST(List, search_for_non_existing_element_returns_false)
{
  emr::list<int, TypeParam> list;
  list.insert(42);
  EXPECT_FALSE(list.search(43));
}

TYPED_TEST(List, search_for_existing_element_returns_true)
{
  emr::list<int, TypeParam> list;
  list.insert(42);
  EXPECT_TRUE(list.search(42));
}

TYPED_TEST(List, remove_existing_element_succeeds)
{
  emr::list<int, TypeParam> list;
  list.insert(42);
  EXPECT_TRUE(list.remove(42));
}

TYPED_TEST(List, remove_nonexisting_element_fails)
{
  emr::list<int, TypeParam> list;
  EXPECT_FALSE(list.remove(42));
}

TYPED_TEST(List, remove_existing_element_twice_fails_the_seond_time)
{
  emr::list<int, TypeParam> list;
  list.insert(42);
  EXPECT_TRUE(list.remove(42));
  EXPECT_FALSE(list.remove(42));
}

namespace
{
#ifdef _DEBUG
  const int MaxIterations = 1000;
#else
  const int MaxIterations = 10000;
#endif
}

TYPED_TEST(List, parallel_usage)
{
  using Reclaimer = TypeParam;
  emr::list<int, TypeParam> list;

  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i)
  {
    threads.push_back(std::thread([i, &list]
    {
      for (int j = 0; j < MaxIterations; ++j)
      {
        typename Reclaimer::region_guard critical_region{};
        EXPECT_FALSE(list.search(i));
        EXPECT_TRUE(list.insert(i));
        EXPECT_TRUE(list.search(i));
        EXPECT_TRUE(list.remove(i));
      }
    }));
  }

  for (auto& thread : threads)
    thread.join();
}

TYPED_TEST(List, parallel_usage_with_same_values)
{
  using Reclaimer = TypeParam;
  emr::list<int, TypeParam> list;

  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i)
  {
    threads.push_back(std::thread([&list]
    {
      for (int j = 0; j < MaxIterations / 10; ++j)
        for (int i = 0; i < 10; ++i)
        {
          typename Reclaimer::region_guard critical_region{};
          list.search(i);
          list.insert(i);
          list.search(i);
          list.remove(i);
        }
    }));
  }

  for (auto& thread : threads)
    thread.join();
}

}