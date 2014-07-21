/* ignore_pending.c - shows the behavior of ignored pending signals.
 *
 * When a process receives a signal that is currently blocked (i.e., is in the
 * process' procmask), it is said to be `pending'. Pending signals are delivered
 * to the process as soon as the signal is unblocked, i.e., removed from the
 * sigprocmask.
 *
 * This program demonstrates that if we choose to ignore pending signals, then
 * they will not reach the process after they are unblocked. To that aim, this
 * program will first block SIGINT, and then raise it; after checking that the
 * signal is indeed in the pending list, SIGINT is then unblocked and we show
 * that the process terminates gracefully, without receiving the signal (there
 * is no custom handler for that signal and the default action is to terminate
 * the process).
 *
 * Usage
 *
 *    $ ./ignore_pending
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 700

#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pexit(const char *fCall);

int
main() {
  sigset_t intSet, pendingSet;

  sigemptyset(&intSet);
  sigaddset(&intSet, SIGINT);

  printf("Blocking SIGINT (%s)\n", strsignal(SIGINT));
  if (sigprocmask(SIG_BLOCK, &intSet, NULL) == -1) {
    pexit("sigprocmask");
  }

  raise(SIGINT);
  printf("Sent SIGINT to self\n");

  if (sigpending(&pendingSet) == -1) {
    pexit("sigpending");
  }

  if (sigismember(&pendingSet, SIGINT)) {
    printf("SIGINT is in the pending list\n");
  } else {
    printf("SIGINT is not in the pending list, aborting\n");
    exit(EXIT_FAILURE);
  }

  printf("Ignoring SIGINT\n");
  if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
    pexit("signal");
  }

  printf("Unblocking SIGINT\n");
  if (sigprocmask(SIG_UNBLOCK, &intSet, NULL) == -1) {
    pexit("sigprocmask");
  }

  if (sigpending(&pendingSet) == -1) {
    pexit("sigpending");
  }

  if (sigismember(&pendingSet, SIGINT)) {
    printf("SIGINT is still in the pending list, aborting\n");
    exit(EXIT_FAILURE);
  } else {
    printf("SIGINT signal is not on the pending list any more\n");
  }

  printf("Program finished successfully.\n");
  exit(EXIT_SUCCESS);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
