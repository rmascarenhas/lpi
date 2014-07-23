/* rt_vs_std_order.c - shows the order in which realtime signals are delivered
 *                     when compared to standard signals.
 *
 * Realtime signals offer more control and predictability when it comes to signal
 * delivery order: they are delivered in order of priority (defined by the signal
 * number), and in the order they were received, for signals with the same number.
 *
 * However, SUSv3 leaves unspecified the order in which signals are delivered to
 * a process when both realtime signals and standard signals are pending. This
 * program can be used to identify how the underlying platform handles this scenario.
 *
 * It does that by changing the disposition and blocking every signal.
 * Then, it sleeps for a given number of seconds, waiting for signals to be sent.
 * The user should send standard and realtime signals during this period. When
 * the process wakes up, the handlers are run (printing information about the
 * handled signal) and the process finishes.
 *
 * Usage
 *
 *    $ ./rt_vs_std_order <sleep-time>
 *    sleep-time - number of seconds the process should sleep.
 *
 * Running this program we learn that Linux first delivers standard signals, and
 * then procedes and delivers realtime signals.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _GNU_SOURCE

#include <limits.h>

#ifndef LONG_MIN
#  include <linux/limits.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void handler(int signal, siginfo_t *sinfo, void *ucontext);

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
#define Fatal(...) {\
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
}

  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  int s;
  char *endptr;
  long sleeptime;

  struct sigaction act;
  sigset_t block_mask, previous_mask;

  sleeptime = strtol(argv[1], &endptr, 10);
  if (sleeptime == LONG_MIN || sleeptime == LONG_MAX || endptr == argv[1]) {
    Fatal("invalid sleep time: %s\n", argv[1]);
  }

  /* change disposition of every signal */
  act.sa_sigaction = handler;
  act.sa_flags = SA_RESTART | SA_SIGINFO;
  sigfillset(&act.sa_mask);

  for (s = 1; s < NSIG; ++s) {
    if (s != SIGSTOP && s != SIGKILL) {
      errno = 0;

      if (sigaction(s, &act, NULL) == -1) {
        /* skip errors related to reserved realtime signals */
        if (errno != EINVAL) {
          pexit("sigaction");
        }
      }
    }
  }

  /* block every signal and sleep */
  if (sigfillset(&block_mask) == -1) {
    pexit("sigfillset");
  }

  if (sigprocmask(SIG_SETMASK, &block_mask, &previous_mask) == -1) {
    pexit("sigprocmask");
  }

  printf("Ready to receive signals. Sleeping for %ld seconds...\n", sleeptime);
  sleep(sleeptime);

  /* unblock signals so that handler can be executed */
  printf("Awwwwn that was a good nap. Unblocking signals now\n");
  if (sigprocmask(SIG_SETMASK, &previous_mask, NULL) == -1) {
    pexit("sigprocmask");
  }

  /* handler should have been successfully executed at this point */
  exit(EXIT_SUCCESS);
}

static void
handler(int signal, siginfo_t *sinfo, __attribute__((unused)) void *ucontext) {
  /* UNSAFE: uses printf(3) */

  printf("\tCaught signal %d (%s)\n", signal, strsignal(signal));
  if (sinfo == NULL) {
    printf("\t sinfo is NULL\n");
  } else {
    printf("\tPID: %ld, data: %d\n", (long) sinfo->si_pid, sinfo->si_value.sival_int);
  }
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <sleep-time>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
