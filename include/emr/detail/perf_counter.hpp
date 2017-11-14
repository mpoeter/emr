#pragma once

namespace emr { namespace detail {
#ifdef WITH_PERF_COUNTER
  struct perf_counter
  {
    perf_counter(size_t& counter) : counter(counter), cnt() {}
    ~perf_counter() { counter += cnt;  }
    void inc() { ++cnt; }
  private:
    size_t& counter;
    size_t cnt;
  };

  #define PERF_COUNTER(name, counter) emr::detail::perf_counter name(counter);
  #define INC_PERF_CNT(counter) ++counter;
#else
  struct perf_counter
  {
    perf_counter() {}
    void inc() {}
  };

  #define PERF_COUNTER(name, counter) emr::detail::perf_counter name;
  #define INC_PERF_CNT(counter)
#endif
}}
