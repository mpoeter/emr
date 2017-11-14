#pragma once

#include <boost/predef.h>

#if BOOST_ARCH_X86
#include <emmintrin.h>
#endif

namespace emr { namespace detail {

struct no_backoff
{
  void operator()() {}
};

class exponential_backoff
{
public:
  void operator()()
  {
    for (unsigned i = 0; i < count; ++i)
      do_backoff();
    count *= 2;
  }
private:
  void do_backoff()
  {
#if BOOST_ARCH_X86
    _mm_pause();
#else
    #warning "No backoff implementation available."
#endif
  }

  unsigned count = 1;
};

using backoff = no_backoff;

}}