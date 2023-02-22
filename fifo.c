#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdatomic.h>
#include <limits.h>
#include "utils.h"
#include "fifo.h"

#define CHECK_FIFO_SIZE() (0 <= MAX_FIFO_SIZE && 0 <= MIN_FIFO_SIZE && MIN_FIFO_SIZE <= MAX_FIFO_SIZE && MAX_FIFO_SIZE < INT_MAX)
#define MIN_FIFO_SIZE (0)
#define MAX_FIFO_SIZE (INT_MAX - 1)


#if defined(FIFO_SEQ)
#define LOAD_NOP(ptr) (*(ptr))
#define STORE_NOP(ptr, val) ((void)((*ptr) = (val)))
#define STORE_SEQ(ptr, val) STORE_NOP(ptr, val)
#define LOAD_SEQ(ptr) LOAD_NOP(ptr)
#define STORE_RLX(ptr, val) STORE_NOP(ptr, val)
#define STORE_REL(ptr, val) STORE_NOP(ptr, val)
#define LOAD_RLX(ptr) LOAD_NOP(ptr)
#define LOAD_ACQ(ptr) LOAD_NOP(ptr)
#define FIFO_ATOMIC_INT _Atomic int
#define FIFO_ATOMIC_ELT _Atomic f_elt_t
#elif defined(FIFO_RLX)
#define STORE_RLX(ptr, val) atomic_store_explicit(ptr, val, memory_order_relaxed)
#define LOAD_RLX(ptr) atomic_load_explicit(ptr, memory_order_relaxed)
#define STORE_SEQ(ptr, val) STORE_RLX(ptr, val)
#define STORE_REL(ptr, val) STORE_RLX(ptr, val)
#define LOAD_SEQ(ptr) LOAD_RLX(ptr)
#define LOAD_ACQ(ptr) LOAD_RLX(ptr)
#define LOAD_NOP(ptr) LOAD_RLX(ptr)
#define STORE_NOP(ptr, val) STORE_RLX(ptr, val)
#define FIFO_ATOMIC_INT _Atomic int
#define FIFO_ATOMIC_ELT _Atomic f_elt_t
#elif defined(FIFO_NAT)
#define LOAD_NOP(ptr) (*(ptr))
#define STORE_NOP(ptr, val) ((void)((*ptr) = (val)))
#define STORE_RLX(ptr, val) STORE_NOP(ptr, val)
#define LOAD_RLX(ptr) LOAD_NOP(ptr)
#define STORE_SEQ(ptr, val) STORE_NOP(ptr, val)
#define STORE_REL(ptr, val) STORE_NOP(ptr, val)
#define LOAD_SEQ(ptr) LOAD_NOP(ptr)
#define LOAD_ACQ(ptr) LOAD_NOP(ptr)
#define FIFO_ATOMIC_INT int
#define FIFO_ATOMIC_ELT f_elt_t
#else
#define STORE_SEQ(ptr, val) atomic_store_explicit(ptr, val, memory_order_seq_cst)
#define LOAD_SEQ(ptr) atomic_load_explicit(ptr, memory_order_seq_cst)
#define STORE_RLX(ptr, val) atomic_store_explicit(ptr, val, memory_order_relaxed)
#define STORE_REL(ptr, val) atomic_store_explicit(ptr, val, memory_order_release)
#define LOAD_RLX(ptr) atomic_load_explicit(ptr, memory_order_relaxed)
#define LOAD_ACQ(ptr) atomic_load_explicit(ptr, memory_order_acquire)
#define LOAD_NOP(ptr) (*((int *)ptr))
#define STORE_NOP(ptr, val) ((void)((*(int *)ptr) = (val)))
#define FIFO_ATOMIC_INT _Atomic int
#define FIFO_ATOMIC_ELT _Atomic f_elt_t
#endif

typedef int64_t f_elt_t;
#define MIN_F_ELT_T INT64_MIN
#define MAX_F_ELT_T INT64_MAX

struct fifo_s {
  int size;
  FIFO_ATOMIC_ELT *buffer;
  FIFO_ATOMIC_INT idx_first;
  FIFO_ATOMIC_INT idx_next;
};
typedef struct fifo_s fifo_t;

#define DIFF_IDX(f, x,y) ((((x) - (y))))
#define INC_IDX(f, x) (((x) + 1) % ((f)->size+1))
#define CURR_IDX(f, x) ((x) % ((f)->size+1))

static int DEBUG;

fifo_p fifo_new(int size)
{
  DEBUG = getenv("FIFO_DEBUG") != NULL;
  VERBOSE(DEBUG, "fifo_new(%d)", size);
  assert(CHECK_FIFO_SIZE());
  assert(size >= MIN_FIFO_SIZE && size <= MAX_FIFO_SIZE);
  
  fifo_p fifo = malloc(sizeof(fifo_t));
  if (fifo == NULL)
    return NULL;
  FIFO_ATOMIC_ELT *buffer = malloc(sizeof(f_elt_t)*(size+1));
  if (buffer == NULL) {
    free(fifo);
    return NULL;
  }
  fifo->size = size;
  fifo->buffer = buffer;
  fifo->idx_first = 0;
  fifo->idx_next = 0;
  return fifo;
}

void fifo_del(fifo_p f)
{
  VERBOSE(DEBUG, "fifo_del(%p)", f);
  assert(f != NULL);
  free(f->buffer);
  free(f);
}

int fifo_push(fifo_p f, int val)
{
  VERBOSE(DEBUG, "fifo_push(%p, %d)", f, val);
  int first = LOAD_RLX(&f->idx_first);
  int next = LOAD_NOP(&f->idx_next);
  int next_next = INC_IDX(f, next);
  if (DIFF_IDX(f, next_next, first) == 0) {
    return -1;
  }
  STORE_RLX(&f->buffer[CURR_IDX(f, next)], (f_elt_t)val);
  STORE_REL(&f->idx_next, next_next);
  return 0;
}

int fifo_first(fifo_p f, int *val_p)
{
  VERBOSE(DEBUG, "fifo_first(%p, %p)", f, val_p);
  int next = LOAD_ACQ(&f->idx_next);
  int first = LOAD_NOP(&f->idx_first);
  if (DIFF_IDX(f, next, first) == 0) {
    return -1;
  }
  f_elt_t val = LOAD_RLX(&f->buffer[CURR_IDX(f, first)]);
  int first_next = INC_IDX(f, first);
  STORE_RLX(&f->idx_first, first_next);
  *val_p = val;
  return 0;
}
