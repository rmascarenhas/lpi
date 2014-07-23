/* null_evp.c - demonstrates the effect of a null event pointer on timer_create(2).
 *
 * The POSIX timer API is more flexible than the traditional UNIX timer API for many
 * reasons. The possibility of creating multiple timers of the same clock per
 * process is one of them.
 *
 * Whenever you pass a NULL pointer to the struct `sigevent' parameter of the
 * `timer_create` system call, the kernel will create a timer that will manifest
 * itself via a SIGALRM signal. This program verifies this behavior.
 *
 * Usage
 *
 *    $ ./null_evp <arg> <seconds>
 *
 *    seconds - the number of seconds after which the timer should be triggered.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE   700
#define _POSIX_C_SOURCE 199309L

#include <limits.h>
#ifndef LONG_MAX
#  include <linux/limits.h>
#endif

#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void sig_handler(int sig, siginfo_t *sinfo, void *ucontext);

#define GetParam(var, param) { \
  char *endptr; \
  var = strtol(param, &endptr, 10); \
  if (var == LONG_MIN || var == LONG_MAX || endptr == param) { \
    fprintf(stderr, "Invalid argument: %s\n", param); \
    exit(EXIT_FAILURE); \
  } \
}

static timer_t tid;

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  long seconds;
  struct itimerspec tspec;

  GetParam(seconds, argv[1]);

  printf("Creating timer with NULL sigevent\n");
  if (timer_create(CLOCK_REALTIME, NULL, &tid) == -1) {
    pexit("timer_create");
  }

  struct sigaction act;
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = sig_handler;

  if (sigaction(SIGALRM, &act, NULL) == -1) {
    pexit("sigaction");
  }

  tspec.it_interval.tv_sec  = 0;
  tspec.it_interval.tv_nsec = 0;

  tspec.it_value.tv_sec  = seconds;
  tspec.it_value.tv_nsec = 0;

  if (timer_settime(tid, 0, &tspec, NULL) == -1) {
    pexit("timer_settime");
  }

  printf("Timer set to trigger after %ld seconds (timer ID %ld).\n", seconds, (long) tid);

  pause(); /* wait for it */

  exit(EXIT_SUCCESS);
}

static void
sig_handler(int sig, siginfo_t *sinfo, __attribute((unused)) void *ucontext) {
  /* UNSAFE: uses printf(3) */

  if (sig != SIGALRM) {
    printf("Received signal %d (%s). Please send only SIGALRM to this program.\n",
      sig, strsignal(sig));

    return;
  }

  int _tid = sinfo->si_value.sival_int;

  printf("\tSIGALRM received\n");
  printf("This means the alarm is set with sigev_notify = SIGEV_SIGNAL and sigev_signo = SIGALRM\n");
  printf("sival_int = %d\n", _tid);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <seconds>.\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
