#pragma once

#include <sys/time.h>
#include <x86intrin.h>

#include "utils.hpp"

inline u64 timeit_syscall(std::function<void()> f) {
  struct timeval start, end;
  gettimeofday(&start, NULL);
  f();
  gettimeofday(&end, NULL);
  return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
}

inline u64 timeit_rdtsc(std::function<void()> f) {
  u64 start = __rdtsc();
  f();
  u64 end = __rdtsc();
  return end - start;
}