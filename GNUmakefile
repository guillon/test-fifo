
# Uncomment one of the 3 options; otherwise defaults to OPT version
#CFLAGS_SEQ=-DFIFO_SEQ
#CFLAGS_RLX=-DFIFO_RLX
#CFLAGS_NAT=-DFIFO_NAT

# Uncomment to activate TS
#CFLAGS_SAN=-g -fsanitize=thread

# Uncomment to force better debuggability
#CFLAGS_DBG=-g -O0

# Uncomment to avoid warning errors on your compiler
CFLAGS_WERROR=-Werror

# Optionally change compiler
CC=gcc

# Below: should not be modified

CFLAGS=-std=gnu11 -O2 -Wall $(CFLAGS_WERROR) $(CFLAGS_DBG) $(CFLAGS_SAN) $(CFLAGS_SEQ) $(CFLAGS_RLX) $(CFLAGS_NAT)
LDFLAGS=$(CFLAGS) -lm
LDFLAGS_test-main-mt=-lpthread

EXES=test-main-seq test-main-delay test-main-mt
EXES_OBJS=$(EXES:%=%.o)

FIFO_HDRS=fifo.h utils.h
FIFO_SRCS=fifo.c utils.c
FIFO_OBJS=$(FIFO_SRCS:%.c=%.o)

OBJS=$(FIFO_OBJS) $(EXES_OBJS)

DEPS=GNUmakefile

all: $(EXES)

$(EXES): %: %.o $(FIFO_OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $* $*.o $(FIFO_OBJS) $(LDFLAGS) $(LDFLAGS_$*)

$(OBJS): %.o: %.c $(FIFO_HDRS) $(DEPS)
	$(CC) $(CFLAGS) $(CFLAGS_$*) -c $*.c -o $*.o

test: all
	./test-main-seq
	./test-main-delay
	./test-main-mt

clean: $(DEPS)
	rm -f $(EXES) $(OBJS)

.SUFFIXES:
.PHONY:
