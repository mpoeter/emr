#pragma once

#include <emr/detail/concurrent_ptr.hpp>
#include <emr/detail/marked_ptr.hpp>
#include <emr/detail/guard_ptr.hpp>
#include "emr/detail/allocation_tracker.hpp"

namespace emr {

  // This is a dummy implementation of the reclamation interface that
  // does NOT reclaim any memory -> THIS RECLAIMER IMPLEMENTATION LEAKS
  // It is only used to measure the raw performance of data structures
  // unaffected by any overhead caused by the reclamation scheme.
  struct dummy
  {
    template <class T, std::size_t N = 0, class DeleterT = std::default_delete<T>>
    class enable_concurrent_ptr
    {
    protected:
      enable_concurrent_ptr() noexcept = default;
      enable_concurrent_ptr(const enable_concurrent_ptr&) noexcept = default;
      enable_concurrent_ptr(enable_concurrent_ptr&&) noexcept = default;
      enable_concurrent_ptr& operator=(const enable_concurrent_ptr&) noexcept = default;
      enable_concurrent_ptr& operator=(enable_concurrent_ptr&&) noexcept = default;
      ~enable_concurrent_ptr() noexcept = default;
    public:
      static constexpr std::size_t number_of_mark_bits = N;
      using Deleter = DeleterT;
    };

    class region_guard {};

    template <class T, class MarkedPtr>
    class guard_ptr;

    template <class T, std::size_t N = T::number_of_mark_bits>
    using concurrent_ptr = emr::detail::concurrent_ptr<T, N, guard_ptr>;

    template <class T, class MarkedPtr>
    class guard_ptr : public detail::guard_ptr<T, MarkedPtr, guard_ptr<T, MarkedPtr>>
    {
      using base = detail::guard_ptr<T, MarkedPtr, guard_ptr>;
      using Deleter = typename T::Deleter;
    public:
      // Guard a marked ptr.
      guard_ptr(const MarkedPtr& p = MarkedPtr()) noexcept : base(p) {}
      explicit guard_ptr(const guard_ptr& p) noexcept : base(p.ptr) {}
      guard_ptr(guard_ptr&& p) noexcept : base(p.ptr) { p.reset(); }

      guard_ptr& operator=(const guard_ptr& p) noexcept { this->ptr = p.ptr; return *this; }
      guard_ptr& operator=(guard_ptr&& p) noexcept { this->ptr = p.ptr; p.reset(); return *this; }

      // Atomically take snapshot of p, and *if* it points to unreclaimed object, acquire shared ownership of it.
      void acquire(concurrent_ptr<T>& p, std::memory_order order = std::memory_order_seq_cst) noexcept
      { this->ptr = p.load(order); };

      // Like acquire, but quit early if a snapshot != expected.
      bool acquire_if_equal(concurrent_ptr<T>& p,
        const MarkedPtr& expected,
        std::memory_order order = std::memory_order_seq_cst) noexcept
      {
        acquire(p, order);
        if (this->ptr != expected)
        {
          reset();
          return false;
        }
        return true;
      }

      // Release ownership. Postcondition: get() == nullptr.
      void reset() noexcept { this->ptr.reset(); }

      // Reset. Deleter d will be applied some time after all owners release their ownership.
      void reclaim(Deleter d = Deleter()) noexcept {}
    };

#ifdef TRACK_ALLOCATIONS
    static emr::detail::allocation_tracker allocation_tracker;
#endif
  };

#ifdef TRACK_ALLOCATIONS
  SELECT_ANY emr::detail::allocation_tracker dummy::allocation_tracker;
#endif
}
