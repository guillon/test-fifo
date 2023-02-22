#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdatomic.h>
#include <limits.h>
#include <pthread.h>
#include "utils.h"
#include "fifo.h"

typedef int64_t f_elt_t;
#define MIN_F_ELT_T INT64_MIN
#define MAX_F_ELT_T INT64_MAX

#define CHECK_FIFO_SIZE() (0 <= MAX_FIFO_SIZE && 0 <= MIN_FIFO_SIZE && MIN_FIFO_SIZE <= MAX_FIFO_SIZE && MAX_FIFO_SIZE <= INT_MAX)
#define MIN_FIFO_SIZE (0)
#define MAX_FIFO_SIZE ((int)(INT_MAX/sizeof(f_elt_t)))

pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&MUTEX)
#define UNLOCK() pthread_mutex_unlock(&MUTEX)

#if defined(FIFO_SEQ)
#define LOAD_NOP(ptr) atomic_load_explicit(ptr, memory_order_seq_cst)
#define STORE_NOP(ptr, val) atomic_store_explicit(ptr, val, memory_order_seq_cst)
#define ADD_NOP(ptr, val) atomic_fetch_add_explicit(ptr, val, memory_order_seq_cst)
#define LOAD_SEQ(ptr) LOAD_NOP(ptr)
#define STORE_SEQ(ptr, val) STORE_NOP(ptr, val)
#define ADD_SEQ(ptr, val) ADD_NOP(ptr, val)
#define LOAD_RLX(ptr) LOAD_NOP(ptr)
#define STORE_RLX(ptr, val) STORE_NOP(ptr, val)
#define ADD_RLX(ptr, val) ADD_NOP(ptr, val)
#define STORE_REL(ptr, val) STORE_NOP(ptr, val)
#define LOAD_ACQ(ptr) LOAD_NOP(ptr)
#define FIFO_ATOMIC_INT _Atomic int
#elif defined(FIFO_RLX)
#define LOAD_RLX(ptr) atomic_load_explicit(ptr, memory_order_relaxed)
#define STORE_RLX(ptr, val) atomic_store_explicit(ptr, val, memory_order_relaxed)
#define ADD_RLX(ptr, val) atomic_fetch_add_explicit(ptr, val, memory_order_relaxed)
#define LOAD_SEQ(ptr) LOAD_RLX(ptr)
#define STORE_SEQ(ptr, val) STORE_RLX(ptr, val)
#define ADD_SEQ(ptr, val) ADD_RLX(ptr, val)
#define LOAD_NOP(ptr) LOAD_RLX(ptr)
#define STORE_NOP(ptr, val) STORE_RLX(ptr, val)
#define ADD_NOP(ptr, val) ADD_RLX(ptr, val)
#define STORE_REL(ptr, val) STORE_RLX(ptr, val)
#define LOAD_ACQ(ptr) LOAD_RLX(ptr)
#define FIFO_ATOMIC_INT _Atomic int
#elif defined(FIFO_NAT)
#define LOAD_NOP(ptr) (*((volatile typeof(*(ptr)) *)(ptr)))
#define STORE_NOP(ptr, val) ((void)({ volatile typeof(*(ptr)) *_p = (ptr); *_p = (val); 0;}))
#define ADD_NOP(ptr, val) ((void)({ volatile typeof(*(ptr)) *_p = (ptr); *_p = *_p + (val); 0;}))
#define LOAD_RLX(ptr) LOAD_NOP(ptr)
#define STORE_RLX(ptr, val) STORE_NOP(ptr, val)
#define ADD_RLX(ptr, val) ADD_NOP(ptr, val)
#define LOAD_SEQ(ptr) LOAD_NOP(ptr)
#define STORE_SEQ(ptr, val) STORE_NOP(ptr, val)
#define ADD_SEQ(ptr, val) ADD_NOP(ptr, val)
#define STORE_REL(ptr, val) STORE_NOP(ptr, val)
#define LOAD_ACQ(ptr) LOAD_NOP(ptr)
#define FIFO_ATOMIC_INT int
#else
#define LOAD_NOP(ptr) (*((volatile typeof(*(ptr)) *)(ptr)))
#define STORE_NOP(ptr, val) ((void)({ volatile typeof(*(ptr)) *_p = (ptr); *_p = (val); 0; }))
#define ADD_NOP(ptr, val) ((void)({ volatile typeof(*(ptr)) *_p = (ptr); *_p = *_p + (val); 0; }))
#define STORE_SEQ(ptr, val) atomic_store_explicit(ptr, val, memory_order_seq_cst)
#define LOAD_SEQ(ptr) atomic_load_explicit(ptr, memory_order_seq_cst)
#define ADD_SEQ(ptr, val) atomic_fetch_add_explicit(ptr, val, memory_order_seq_cst)
#define LOAD_RLX(ptr) atomic_load_explicit(ptr, memory_order_relaxed)
#define STORE_RLX(ptr, val) atomic_store_explicit(ptr, val, memory_order_relaxed)
#define ADD_RLX(ptr, val) atomic_fetch_add_explicit(ptr, val, memory_order_relaxed)
#define STORE_REL(ptr, val) atomic_store_explicit(ptr, val, memory_order_release)
#define LOAD_ACQ(ptr) atomic_load_explicit(ptr, memory_order_acquire)
#define FIFO_ATOMIC_INT int
#endif

struct fifo_s {
  int size;
  f_elt_t *buffer;
  FIFO_ATOMIC_INT idx_first;
  FIFO_ATOMIC_INT num;
};
typedef struct fifo_s fifo_t;

#define INC_IDX(f, x) ((int)({ int _x = (x), _s = (f)->size; _x == _s - 1 ? 0: _x + 1;}))
#define ADD_IDX(f, x, n) ((int)({ int _s = (f)->size, _p = (x) + ((n) - _s); _p < 0 ? _p + _s: _p;}))

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
  f_elt_t *buffer = malloc(sizeof(f_elt_t)*(size));
  if (buffer == NULL) {
    free(fifo);
    return NULL;
  }
  fifo->size = size;
  fifo->buffer = buffer;
  fifo->idx_first = 0;
  fifo->num = 0;
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
  int num = LOAD_RLX(&f->num);
  if (num == f->size) {
    return -1;
  }
  LOCK();
  num = LOAD_RLX(&f->num);
  int first = LOAD_RLX(&f->idx_first);
  STORE_RLX(&f->buffer[ADD_IDX(f, first, num)], (f_elt_t)val);
  ADD_RLX(&f->num, 1);
  UNLOCK();
  return 0;
}

int fifo_first(fifo_p f, int *val_p)
{
  VERBOSE(DEBUG, "fifo_first(%p, %p)", f, val_p);
  int num = LOAD_RLX(&f->num);
  if (num == 0) {
    return -1;
  }
  LOCK();
  num = LOAD_RLX(&f->num);
  int first = LOAD_RLX(&f->idx_first);
  f_elt_t val = LOAD_RLX(&f->buffer[first]);
  int first_next = INC_IDX(f, first);
  STORE_RLX(&f->idx_first, first_next);
  ADD_RLX(&f->num, -1);
  UNLOCK();
  *val_p = val;
  return 0;
}
