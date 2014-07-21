/* rtsig_sender.c - send realtime signals.
 *
 * This is an utility program that sends realtime signals to a given process
 * passing along an integer value as data and can send signals multiple times.
 *
 * Usage
 *
 *    $ ./rtsig_sender <pid> <sig> <data> [num-sigs]
 *    pid      - the PID of the receiver
 *    sig      - the signal number
 *    data     - the data to be sent alongside the signal (must be an integer)
 *    num-sigs - (optional) number of times the signal should be sent. If sent
 *               multiple times, the data is incremented each time.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _POSIX_C_SOURCE 199309L /* enables sigqueue */

#include <limits.h>

#ifndef LONG_MIN
#  include <linux/limits.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static long extract_long(const char *longstr, const char *var);

int
main(int argc, char *argv[]) {
  if (argc < 4) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  long pid, signal, data, num_sigs;
  int i;

  union sigval sv;

  num_sigs = 1;

  pid    = extract_long(argv[1], "PID");
  signal = extract_long(argv[2], "signal");
  data   = extract_long(argv[3], "data argument");

  if (argc > 4) {
    num_sigs = extract_long(argv[4], "num-signals argument");
  }

  /* Display process PID and UID for help debugging of the receiver */
  printf("%s: PID: %ld, UID %ld\n", argv[0], (long) getpid(), (long) getuid());

  for (i = 0; i < num_sigs; ++i) {
    sv.sival_int = data + i;
    if (sigqueue(pid, signal, sv) == -1) {
      pexit("sigqueue");
    }
  }

  exit(EXIT_SUCCESS);
}

static long
extract_long(const char *longstr, const char *var) {
#define Fatal(...) { \
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
}

  char *endptr;
  long l;

  l = strtol(longstr, &endptr, 10);
  if (l == LONG_MIN || l == LONG_MAX || endptr == longstr) {
    Fatal("invalid %s %s\n", var, longstr);
  }

  return l;
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <pid> <sig> <data> [num-sigs]\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
