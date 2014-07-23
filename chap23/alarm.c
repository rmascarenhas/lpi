/* alarm.c - implements the alarm(2) system call on user level.
 *
 * The `alarm(2)` system call allows the calling process to set up an interval
 * after which a SIGALRM will be sent to itself. This program implements
 * this functionality in user level on top of the `setitimer(2)` system call.
 *
 * To demonstrate that the `alarm' implementation correctly removes the previous
 * timer, returning the remaining time for that old alarm to finsh, you can send
 * SIGINT signals to this program (Ctrl-C on most shells), which will cause the
 * alarm to be armed again.
 *
 * Usage
 *
 *    $ ./alarm <seconds>
 *
 *    seconds - the number of seconds after which the process should be alarmed.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE

#include <limits.h>
#ifndef LONG_MAX
#  include <linux/limits.h>
#endif

#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

enum { FALSE, TRUE };

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void sig_handler(int sig);
static unsigned int _alarm(unsigned int seconds);

static volatile sig_atomic_t got_alarm = FALSE;

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  long seconds;
  char *endptr;
  unsigned int remaining_alrm;
  struct sigaction act;

  seconds = strtol(argv[1], &endptr, 10);
  if (seconds == LONG_MIN || seconds == LONG_MAX || endptr == argv[1]) {
    fprintf(stderr, "Invalid argument: %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  act.sa_handler = sig_handler;
  if (sigaction(SIGINT, &act, NULL) == -1) {
    pexit("sigaction");
  }

  if (sigaction(SIGALRM, &act, NULL) == -1) {
    pexit("sigaction");
  }

  for (;;) {
    remaining_alrm = _alarm(seconds);
    if (remaining_alrm == 0) {
      /* this should run on the first iteration */
      printf("Alarm set up to ring in %ld seconds.\n", seconds);
    } else {
      printf("Alarm re-scheduled. Previous alarm would ring in %d seconds.\n", remaining_alrm);
    }

    pause();
    if (got_alarm) {
      break;
    }
  }

  exit(EXIT_SUCCESS);
}

static unsigned int
_alarm(unsigned int seconds) {
  struct itimerval newtimer, oldtimer;

  newtimer.it_interval.tv_sec  = 0;
  newtimer.it_interval.tv_usec = 0;

  newtimer.it_value.tv_sec  = seconds;
  newtimer.it_value.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &newtimer, &oldtimer) == -1) {
    return INT_MAX;
  }

  return oldtimer.it_value.tv_sec;
}

static void
sig_handler(int sig) {
  if (sig == SIGINT) {
    /* do nothing and go back to the main loop, causing the alarm to be
     * set again */
    return;

  } else if (sig == SIGALRM) {
    /* UNSAFE: uses printf(3) */
    printf("\tSIGALRM received\n");
    got_alarm = TRUE;
  }
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <seconds>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
