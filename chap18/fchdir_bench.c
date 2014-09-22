/* fchdir_bench.c - benchmark chdir vs fchdir.
 *
 * This is a dummy program to be used to benchmark the operation of changing
 * the current working directory temporarily, perform some operation, and then
 * restore it. To achieve this behavior, both the `chdir` and the `fchdir` system
 * calls can be used.
 *
 * To benchmark their performance, this program can be told to perform the above
 * operation a number of times. The time spent can then be measured using tools
 * such as the UNIX `time(1)` command.
 *
 * Usage
 *
 *    $ ./fchdir_bench <numops> <syscall>
 *
 *    numops  - the number of operations to be performed.
 *    syscall - can be either c or f, to denote chdir or fchdir, respectively
 *
 * Running this program to perform the operation a milion times, the results were
 * mostly around the values of:
 *
 *    % time ./fchdir_bench 1000000 c
 *    Done. Performed 1000000 chdir operations
 *
 *    real  0m3.822s
 *    user  0m0.152s
 *    sys 0m3.664s
 *
 *    % time ./fchdir_bench 1000000 f
 *    Done. Performed 1000000 fchdir operations
 *
 *    real  0m2.997s
 *    user  0m0.168s
 *    sys 0m2.772s
 *
 * As can be seen, performing the `fchdir(2)` calls is faster and the difference
 * increases as the number of operation rises. This happens since we do not need
 * to get the current working directory on every operation and also because
 * manipulating number identifiers (file descriptors) is faster than doing so with
 * strings.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 500

#include <limits.h>

#ifndef PATH_MAX
#  include <linux/limits.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CHDIR, FCHDIR };

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void chdirOperation();
static void fchdirOperation();

int
main(int argc, char *argv[]) {
  if (argc != 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  long numops;
  int syscall, i;
  char *endptr;

  numops = strtol(argv[1], &endptr, 10);
  if (numops == 0 || numops == LONG_MIN || numops == LONG_MAX) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  if (!strncmp(argv[2], "c", 2)) {
    syscall = CHDIR;
  } else if (!strncmp(argv[2], "f", 2)) {
    syscall = FCHDIR;
  } else {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  for (i = 0; i < numops; ++i) {
    if (syscall == CHDIR) {
      chdirOperation();
    } else {
      fchdirOperation();
    }
  }

  printf("Done. Performed %ld %s operations\n", numops, syscall == CHDIR ? "chdir" : "fchdir");

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <numops> <syscall>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

static void
chdirOperation() {
  char cwd[PATH_MAX];

  /* get current working directory */
  if (getcwd(cwd, PATH_MAX) == NULL) {
    pexit("getcwd");
  }

  /* temporarily change directory */
  if (chdir("..") == -1) {
    pexit("chdir");
  }

  /* perform some operation */
  struct stat sBuf;
  if (stat(".", &sBuf) == -1) {
    pexit("stat");
  }

  /* restor current working directory */
  if (chdir(cwd) == -1) {
    pexit("chdir");
  }
}

static void
fchdirOperation() {
  int fd;

  /* get current working directory */
  fd = open(".", O_RDONLY);
  if (fd == -1) {
    pexit("open");
  }

  /* temporarily change directory */
  if (chdir("..") == -1) {
    pexit("chdir");
  }

  /* perform some operation */
  struct stat sBuf;
  if (stat(".", &sBuf) == -1) {
    pexit("stat");
  }

  /* restor current working directory */
  if (fchdir(fd) == -1) {
    pexit("chdir");
  }

  if (close(fd) == -1) {
    pexit("close");
  }
}
