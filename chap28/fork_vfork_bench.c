/* fork_vfork_bench.c - executes a benchmark of the fork(2) and vfork(2) system calls.
 *
 * Depending on the intended usage, on the caller process and  other factors, the
 * `vfork(2)` system call can create a significant performance improvement over
 * the traditional `fork(2)` syscall (though with its inherent drawbacks).
 *
 * This program attempts to benchmark both system calls by performing a series
 * of consecutive process creations (by default 10,000 - but can be overwritten
 * by defining BENCH_RUNS when compiling). The caller process will previously
 * allocate a certain amount of memory so that page table copying time can be
 * taken into account (by default, 1 MB of memory is allocated, but that can be
 * changed by defining the CALLER_HEAP variable to the number of KB to be allocated
 * by the caller). The `time(1)` utility can be used to extract useful information
 * on total and CPU times.
 *
 * Usage
 *
 *    $ ./fork_vfork_bench [vfork]
 *
 *    vfork - if given, processes will be created using `vfork(2)` instead of
 *    the default `fork(2)`.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef BENCH_RUNS
#  define BENCH_RUNS (10000)
#endif

#ifndef CALLER_HEAP
#  define CALLER_HEAP (1024)
#endif

#define SyscallStr(b) (b ? "fork" : "vfork")

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void *growHeap();
static void doBench(int forkSyscall);

int
main(int argc, char *argv[]) {
  if (argc > 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  void *mem = growHeap();
  int forkSyscall = argv[1] == NULL;

  printf("Benchmarking %s\n", SyscallStr(forkSyscall));
  doBench(forkSyscall);
  free(mem);

  exit(EXIT_SUCCESS);
}

static void
doBench(int forkSyscall) {
  int i, status;

  for (i = 0; i < BENCH_RUNS; ++i) {
    switch (forkSyscall ? fork() : vfork()) {
      case -1:
        pexit(SyscallStr(forkSyscall));
        break;

      case 0:
        /* child: immediately exits */
        _exit(EXIT_SUCCESS);
        break;

      default:
        /* parent: wait for the child and continue */
        if (wait(&status) == -1) {
          pexit("wait");
        }
    }
  }
}

/* grows the process heap as defined by the CALLER_HEAP constant */
static void *
growHeap() {
  void *mem;

  mem = malloc(CALLER_HEAP * 1024);
  if (mem == NULL) {
    pexit("malloc");
  }

  return mem;
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s [syscall]\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
