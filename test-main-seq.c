#include <stdio.h>
#include <assert.h>
#include "fifo.h"

#define N_FIFO 16
#define FIFO_SIZE 8
int main(int argc, char *argv[])
{
  int r;
  int val;
  fifo_p fifos[N_FIFO];
  for (int i = 0; i < N_FIFO; i++) {
    fifos[i] = fifo_new(FIFO_SIZE);
    assert(fifos[i] != NULL);
  }

  for (int i = 0; i < N_FIFO; i++) {
    fifo_p fifo = fifos[i];
    for (int e = 0; e < FIFO_SIZE; e++) {
      r = fifo_push(fifo, e);
      assert(r == 0);
    }
    r = fifo_push(fifo, 0);
    assert(r != 0);
  }
  for (int i = 0; i < N_FIFO; i++) {
    fifo_p fifo = fifos[i];
    for (int e = 0; e < FIFO_SIZE; e++) {
      r = fifo_first(fifo, &val);
      assert(r == 0);
      assert(val == e);
    }
    r = fifo_first(fifo, &val);
    assert(r != 0);
  }
  for (int i = 0; i < N_FIFO; i++) {
    fifo_del(fifos[i]);
  }
  return 0;
}
