#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include "utils.h"
#include "fifo.h"

static int DEBUG;

static int produce(fifo_p fifo, double period, double delay, int *errors_p)
{
  double start = dtime(), now = start;
  int num = 0;
  int r;
  int val = 0;
  VERBOSE(DEBUG, "produce(%p, %.2f): start: %.2f", fifo, period, start);
  while (period <= 0 || now <= start + period) {
    r = fifo_push(fifo, val);
    if (r == 0) {
      val = (val + 1) % INT_MAX;
      num += 1;
    } else {
      VERBOSE(DEBUG, "produce(%p, %.2f): full", fifo, period);
    }
    if (delay > 0)
      dsleep(delay);
    now = dtime();
  }
  VERBOSE(DEBUG, "produce(%p, %.2f): exit: %d", fifo, period, num);
  if (errors_p)
    *errors_p = 0;
  return num;
}

static int consume(fifo_p fifo, double period, double delay, int *errors_p)
{
  double start = dtime(), now = start;
  int num = 0, errors = 0;
  int r;
  int val;
  int expected = 0;
  VERBOSE(DEBUG, "consume(%p, %.2f)", fifo, period);
  while(period <= 0 || now <= start + period) {
    r = fifo_first(fifo, &val);
    if (r == 0) {
      errors += val != expected;
      expected = (expected + 1) % INT_MAX;
      num += 1;
    } else {
      VERBOSE(DEBUG, "consume(%p, %.2f): empty", fifo, period);
    }
    if (delay > 0)
      dsleep(delay);
    now = dtime();
  }
  VERBOSE(DEBUG, "consume(%p, %.2f): exit: %d", fifo, period, num);
  if (errors_p)
    *errors_p = errors;
  return num;
}

typedef struct {
  int id;
  fifo_p *fifos;
  int n_fifos;
  int fifo_id;
  double period;
  double delay;
  int num;
  int errors;
} thread_args_t;


static void *thread_produce(void *data) {
  thread_args_t *args = (thread_args_t *)data;
  VERBOSE(DEBUG, "thread_produce(%d): fifo_id: %d", args->id, args->fifo_id);
  fifo_p fifo = args->fifos[args->fifo_id];
  int errors;
  int num = produce(fifo, args->period, args->delay, &errors);
  args->num = num;
  args->errors = errors;
  return (void *)(intptr_t)(args->errors != 0);
}
  
static void *thread_consume(void *data) {
  thread_args_t *args = (thread_args_t *)data;
  fifo_p fifo = args->fifos[args->fifo_id];
  int errors;
  int num = consume(fifo, args->period, args->delay, &errors);
  args->num = num;
  args->errors = errors;
  return (void *)(intptr_t)(args->errors != 0);
}
  

#define N_FIFO 16
#define FIFO_SIZE 8
#define N_THREADS 2*N_FIFO
int main(int argc, char *argv[])
{
  int r;
  fifo_p fifos[N_FIFO];
  pthread_t threads[N_THREADS];
  thread_args_t thread_args[N_THREADS];
  
  DEBUG = getenv("MAIN_DEBUG") != NULL;

  for (int i = 0; i < N_FIFO; i++) {
    fifos[i] = fifo_new(FIFO_SIZE);
    assert(fifos[i] != NULL);
  }

  double period = 0.3, p_delay = 0, c_delay = 0;

  double elapsed = dtime();
  for (int i = 0; i < N_FIFO; i++) {
    thread_args_t produce_args = { i*2, fifos, N_FIFO, i, period, p_delay };
    thread_args_t consume_args = { i*2+1, fifos, N_FIFO, i, period, c_delay };
    thread_args[i*2] = produce_args;
    thread_args[i*2+1] = consume_args;
    r = pthread_create(&threads[i*2], NULL, thread_produce, &thread_args[i*2]);
    assert(r == 0);
    r = pthread_create(&threads[i*2+1], NULL, thread_consume, &thread_args[i*2+1]);
    assert(r == 0);
  }

  int errors = 0;
  int consumed = 0, c_errors = 0;
  int produced = 0, p_errors = 0;
  for (int i = 0; i < N_FIFO; i++) {
    void *p_ret;
    pthread_join(threads[i*2], &p_ret);
    errors += (intptr_t)p_ret;
    p_errors += thread_args[i*2].errors;
    produced += thread_args[i*2].num;
    pthread_join(threads[i*2+1], &p_ret);
    errors += (intptr_t)p_ret;
    c_errors += thread_args[i*2+1].errors;
    consumed += thread_args[i*2+1].num;
  }
  elapsed = dtime() - elapsed;

  for (int i = 0; i < N_FIFO; i++) {
    fifo_del(fifos[i]);
  }

  if (errors) {
    fprintf(stderr, "ERROR: %d threads have errors: p_errors: %d/%d, c_errors: %d/%d (%.2f op/sec)\n", errors, p_errors, produced, c_errors, consumed, (consumed+produced)/elapsed);
  } else {
    assert(p_errors == 0);
    assert(c_errors == 0);
    fprintf(stdout, "SUCCESS: no error reported: p_errors: %d/%d, c_errors: %d/%d (%.2f op/sec)\n", p_errors, produced, c_errors, consumed, (consumed+produced)/elapsed);;
  }
  return errors != 0;
}
