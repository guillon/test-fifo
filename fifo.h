#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdint.h>
typedef struct fifo_s *fifo_p;

extern fifo_p fifo_new(int size);
extern void fifo_del(fifo_p fifo);
extern int fifo_push(fifo_p fifo, int val);
extern int fifo_first(fifo_p fifo, int *val);

#endif
