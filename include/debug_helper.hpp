#ifdef WIN32
#include <Windows.h>

#undef assert
#define assert(expression) if (!(expression)) __debugbreak();

#define LOG(msg, ...)

#ifndef LOG
#define LOG(msg, ...) { \
  char buf[128]; \
  sprintf(buf, "% u - %s\n", GetCurrentThreadId(), msg); \
  printf(buf, __VA_ARGS__); \
  fflush(stdout); }
#endif
#else
  #undef assert
  #define assert(expr) if (!(expr)) { fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); __builtin_trap(); }
#endif
