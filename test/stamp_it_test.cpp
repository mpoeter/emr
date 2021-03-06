#include <emr/stamp_it.hpp>

#include <gtest/gtest.h>

namespace {

using Reclaimer = emr::stamp_it;

struct Foo : Reclaimer::enable_concurrent_ptr<Foo, 2>
{
  Foo** instance;
  Foo(Foo** instance) : instance(instance) {}
  virtual ~Foo() { if (instance) *instance = nullptr; }
};

template <typename T>
using concurrent_ptr = Reclaimer::concurrent_ptr<T>;
template <typename T> using marked_ptr = typename concurrent_ptr<T>::marked_ptr;

struct StampIt : testing::Test
{
  Foo* foo = new Foo(&foo);
  marked_ptr<Foo> mp = marked_ptr<Foo>(foo, 3);

  void update_epoch()
  {
    // UpdateThreshold is set to 0, so we simply need create a guard_ptr to some dummy object
    // to trigger and epoch update.
    Foo dummy(nullptr);
    concurrent_ptr<Foo>::guard_ptr gp(&dummy);
  }

  void wrap_around_epochs()
  {
    update_epoch();
    update_epoch();
    update_epoch();
  }

  void TearDown() override
  {
    
  }
};

TEST_F(StampIt, mark_returns_the_same_mark_as_the_original_marked_ptr)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  EXPECT_EQ(mp.mark(), gp.mark());
}

TEST_F(StampIt, get_returns_the_same_pointer_as_the_original_marked_ptr)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  EXPECT_EQ(mp.get(), gp.get());
}

TEST_F(StampIt, reset_releases_ownership_and_sets_pointer_to_null)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  gp.reset();
  EXPECT_EQ(nullptr, gp.get());
}

TEST_F(StampIt, reclaim_releases_ownership_and_the_object_gets_deleted)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  gp.reclaim();
  EXPECT_EQ(nullptr, foo);
  EXPECT_EQ(nullptr, gp.get());
}

TEST_F(StampIt, object_cannot_be_reclaimed_as_long_as_another_guard_protects_it)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  concurrent_ptr<Foo>::guard_ptr gp2(mp);
  gp.reclaim();
  wrap_around_epochs();
  EXPECT_NE(nullptr, foo);
}

TEST_F(StampIt, copy_constructor_leads_to_shared_ownership_preventing_the_object_from_beeing_reclaimed)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  concurrent_ptr<Foo>::guard_ptr gp2(gp);
  gp.reclaim();
  wrap_around_epochs();
  EXPECT_NE(nullptr, foo);
}

TEST_F(StampIt, move_constructor_moves_ownership_and_resets_source_object)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  concurrent_ptr<Foo>::guard_ptr gp2(std::move(gp));
  gp2.reclaim();
  wrap_around_epochs();
  EXPECT_EQ(nullptr, gp.get());
  EXPECT_EQ(nullptr, foo);
}

TEST_F(StampIt, copy_assignment_leads_to_shared_ownership_preventing_the_object_from_beeing_reclaimed)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  concurrent_ptr<Foo>::guard_ptr gp2{};
  gp2 = gp;
  gp.reclaim();
  wrap_around_epochs();
  EXPECT_NE(nullptr, foo);
}

TEST_F(StampIt, move_assignment_moves_ownership_and_resets_source_object)
{
  concurrent_ptr<Foo>::guard_ptr gp(mp);
  concurrent_ptr<Foo>::guard_ptr gp2{};
  gp2 = std::move(gp);
  gp2.reclaim();
  wrap_around_epochs();
  EXPECT_EQ(nullptr, gp.get());
  EXPECT_EQ(nullptr, foo);
}
}
