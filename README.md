Single Prod/Cons FIFO Test
==========================

This harness tests a single producer/single consummer
FIFO queue implemented with lock-free buffer access.

On non-TSO (Totally Store Ordered) machines (i.e. ARM)
it is required to explicit
the happens-before relationship between the update
of the buffer and the index in the buffer on the
producer side as well as the read of the buffer index
and the read of the buffer on the consumer side.

On TSO machines (i.e. X86) this is not necessary, though
it is still necessary to perform atomic load store
operations in order to ensure inter-threads coherencies
of values writes and reads.

For portability, the semi-barriers in the optimised case (OPT)
are implemented `atomic_...()` primitives, as for instance
in this snippet:

    int last;
    int first;

    producer:
      l = last; 
      atomic_store_explicit(&buffer[l], val, memory_order_relaxed);
      // \- happens-before store of last-\
      atomic_store_explicit(&last, l + 1, memory_order_release);

      ... 
    consumer:
      l = atomic_load_explicit(&last, memory_order_acquire);
      // \- happens-before load of val -\
      if (first < l)
        val = atomic_load_explicit(&buffer[first++], memory_order_relaxed);

Other variants use the C11 `_Atomic` type, though it is not
strictly necessary, as this just tells the compiler to always
used an explicit `memory_order_seq_cst` access for any dereference
of the location.

Variants
--------

There are four variants to the `fifo.c` implementation:
- OPT: the default, correct and fastest
- SEQ: `-DFIFO_SEQ`, correct but always sequentially consistent
- RLX: `-DFIFO_RLX`, incorrect, using relaxed, though atomic operations
- NAT: `-DFIFO_NAT`, incorrect, uses natutal load/store accesses

Compile and run with:

    make all
    ./test-main-ts

Change options within makefile for modifying the variant
and/or to activate sanitizer.

Behavior of Thread Sanitizer
----------------------------

TS report on X86, all OK, this is correct.

The TS reports on ARM (aarch64, 4 cores) are:
- OPT: OK
- SEQ: OK
- NAT: FAILED: this is correct
- RLX: OK : this is incorrect, it is unable to detect the errors

This seems reasonable as the TS algorithms is not able to detect
happens-before relationsships which should be present for the correct
program behavior.

Behavior of RR
--------------

RR report on X86, all OK, this is correct.

Actually RR not tested (not available as package) on ARM.

TODO: is RR ported to ARM?

Though as RR executes in single threaded mode, it is probably not possible for it
to find a schedule that will fail.


Behavior of Hermit
------------------

TODO: test Hermit

As Hermit maintains several threads, it may be possible that errors are exhibited,
though for the reproducibility, it would imply a preservation of the dispatch
of threads to the different CPUs, not sure this is implemented.


