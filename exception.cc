/* Copyright 2026 MarcosHCK
 */
#include <exception.h>
using namespace utility;

std::stacktrace with_stacktrace::current (size_t skip)
{
  return std::stacktrace::current (1 + skip);
}

std::stacktrace with_stacktrace::current (size_t skip, size_t max_depth)
{
  return std::stacktrace::current (1 + skip, max_depth);
}
