
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
#CC=clang

# Below: should not be modified

CFLAGS=-std=gnu11 -Wall -O2 $(CFLAGS_WERROR) $(CFLAGS_DBG) $(CFLAGS_SAN) $(CFLAGS_SEQ) $(CFLAGS_RLX) $(CFLAGS_NAT)
LDFLAGS=$(CFLAGS) -lm
LDFLAGS_test-main-mt=-lpthread
LDFLAGS_test-main-nlf-mt=-lpthread

EXES=test-main-seq test-main-delay test-main-mt test-main-nlf-seq test-main-nlf-delay test-main-nlf-mt
EXES_OBJS=$(EXES:%=%.o)
EXE_OBJS_test-main-seq=test-main-seq.o $(FIFO_OBJS)
EXE_OBJS_test-main-delay=test-main-delay.o $(FIFO_OBJS)
EXE_OBJS_test-main-mt=test-main-mt.o $(FIFO_OBJS)
EXE_OBJS_test-main-nlf-seq=test-main-seq.o $(FIFO_NLF_OBJS)
EXE_OBJS_test-main-nlf-delay=test-main-delay.o $(FIFO_NLF_OBJS)
EXE_OBJS_test-main-nlf-mt=test-main-mt.o $(FIFO_NLF_OBJS)

FIFO_HDRS=fifo.h
FIFO_SRCS=fifo.c
FIFO_OBJS=$(FIFO_SRCS:%.c=%.o)

FIFO_NLF_HDRS=fifo.h
FIFO_NLF_SRCS=fifo-nlf.c
FIFO_NLF_OBJS=$(FIFO_NLF_SRCS:%.c=%.o)

UTILS_HDRS=utils.h
UTILS_SRCS=utils.c
UTILS_OBJS=$(UTILS_SRCS:%.c=%.o)

OBJS=$(FIFO_OBJS) $(FIFO_NLF_OBJS) $(EXES_OBJS) $(UTILS_OBJS)

DEPS=GNUmakefile

all: $(EXES)

.SUFFIXES:
.PHONY:
.SECONDEXPANSION:

$(EXES): %: $$(EXE_OBJS_$$*) $(UTILS_OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $* $(EXE_OBJS_$*) $(UTILS_OBJS) $(LDFLAGS) $(LDFLAGS_$*)

$(OBJS): %.o: %.c $(FIFO_HDRS) $(UTILS_HDRS) $(DEPS)
	$(CC) $(CFLAGS) $(CFLAGS_$*) -c $*.c -o $*.o

test: test-lf test-nlf

test-lf: all
	./test-main-seq
	./test-main-delay
	./test-main-mt

test-nlf: all
	./test-main-nlf-seq
	./test-main-nlf-delay
	./test-main-nlf-mt

clean: $(DEPS)
	rm -f $(EXES) $(OBJS)

