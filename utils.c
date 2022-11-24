#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdarg.h>
#include "utils.h"

double dtime()
{
  struct timespec tv;
  int r = clock_gettime(CLOCK_MONOTONIC_RAW, &tv);
  assert(r == 0);
  return tv.tv_sec + (double)tv.tv_nsec / 1000000000;
}

int dsleep(double delay)
{
  assert(delay >= 0 && delay <= INT_MAX);
  struct timespec ts, rem;
  double secs = (int)delay;
  double nsecs = (delay - secs) * 1e9;
  int r;
  ts.tv_sec = (time_t)secs;
  ts.tv_nsec = (long)nsecs % 1000000000;
  do {
    r = nanosleep(&ts, &rem);
    if (r == 0 || errno != EINTR)
      break;
    ts = rem;
  } while (1);
  return r;
}

int verbose(int level, const char *name, int line, const char *msg, ...)
{
  va_list vargs;
  static __thread char buffer[256];
  int n, size = (sizeof(buffer)/sizeof(*buffer));
  long tid = syscall(SYS_gettid), pid = getpid();

  va_start(vargs, msg);
  if (tid != pid) {		
    n = snprintf(buffer, size, "DEBUG: %s:%d: tid: %ld: ", name, line, tid);
  } else {
    n = snprintf(buffer, size, "DEBUG: %s:%d: ", name, line);
  }							
  if (n < size)
    n += vsnprintf(buffer + n, size - n, msg, vargs);
  if (n > size - 2)			
    n = size - 2;
  n +=snprintf(buffer + n, size - n, "\n");
  fputs(buffer, stderr);
  va_end(vargs);
  return n;
}
