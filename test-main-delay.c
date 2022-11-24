#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include "utils.h"
#include "fifo.h"

static int DEBUG;

static void produce(fifo_p fifo, double period)
{
  double start = dtime();
  int r;
  int val = 0;
  VERBOSE(DEBUG, "produce(%p, %.2f)", fifo, period);
  while (period == 0 || dtime() < start + period) {
    r = fifo_push(fifo, val);
    if (r == 0) {
      val = (val + 1) % INT_MAX;
    } else {
      VERBOSE(DEBUG, "produce(%p, %.2f): sleep: %.2f", fifo, period, period);
      dsleep(period);
    }
  }
}

static void consume(fifo_p fifo, double period)
{
  double start = dtime();
  int r;
  int val;
  int expected = 0;
  VERBOSE(DEBUG, "consume(%p, %.2f)", fifo, period);
  while(period == 0 || dtime() < start + period) {
    r = fifo_first(fifo, &val);
    if (r == 0) {
      assert(val == expected);
      expected = (expected + 1) % INT_MAX;
    } else {
      VERBOSE(DEBUG, "consume(%p, %.2f): sleep: %.2f", fifo, period, period);
      dsleep(period);
    }
  }
}


#define N_FIFO 16
#define FIFO_SIZE 8
int main(int argc, char *argv[])
{
  fifo_p fifos[N_FIFO];

  DEBUG = getenv("MAIN_DEBUG") != NULL;
  
  for (int i = 0; i < N_FIFO; i++) {
    fifos[i] = fifo_new(FIFO_SIZE);
    assert(fifos[i] != NULL);
  }

  for (int i = 0; i < N_FIFO; i++) {
    fifo_p fifo = fifos[i];
    produce(fifo, 0.01);
  }
  for (int i = 0; i < N_FIFO; i++) {
    fifo_p fifo = fifos[i];
    consume(fifo, 0.01);
  }
  for (int i = 0; i < N_FIFO; i++) {
    fifo_del(fifos[i]);
  }
  return 0;
}
