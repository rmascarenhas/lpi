/* sigcont.c - demonstration of assigning handlers and blocking SIGCONT.
 *
 * The SIGCONT signal, when sent to a process, causes it to be schedulable
 * again if it was suspended. This signal cannot be ignored, so that even if
 * you block this signal and suspend the process, SIGCONT will still wake
 * up the process.
 *
 * However, if the signal is blocked, then its handler, if set, will be invoked
 * only when it is unblocked. That is what this program demonstrates.
 *
 * Usage
 *
 *    $ ./sigcont
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { FALSE, TRUE } Bool;

static volatile Bool got_tstp = FALSE;

static void suspended(int signal);
static void continued(int signal);

static void pexit(const char *fCall);

int
main() {
  sigset_t cont_set;
  struct sigaction st_suspended, st_continued;

  /* add a handler for SIGTSTP */
  st_suspended.sa_handler = suspended;
  st_suspended.sa_flags = SA_RESETHAND;
  if (sigaction(SIGTSTP, &st_suspended, NULL) == -1) {
    pexit("sigaction");
  }

  /* add a handler for SIGCONT and then block it */
  st_continued.sa_handler = continued;
  if (sigaction(SIGCONT, &st_continued, NULL) == -1) {
    pexit("sigaction");
  }

  if (sigemptyset(&cont_set) == -1) {
    pexit("sigemptyset");
  }

  if (sigaddset(&cont_set, SIGCONT) == -1) {
    pexit("sigaddset");
  }

  if (sigprocmask(SIG_BLOCK, &cont_set, NULL) == -1) {
    pexit("sigprocmask");
  }

  printf("SIGCONT (%s) blocked. Please suspend me (Ctrl-Z on most shells)\n", strsignal(SIGCONT));
  while (!got_tstp) {
    pause();
  }

  printf("I am back. However, the SIGCONT handler was not run yet. I will unblock it and now.\n");
  if (sigprocmask(SIG_UNBLOCK, &cont_set, NULL) == -1) {
    pexit("sigprocmask");
  }

  printf("That is it. Bye.\n");
  exit(EXIT_SUCCESS);
}

static void
suspended(__attribute__((unused)) int signal) {
  /* UNSAFE: uses printf(3) */

  got_tstp = TRUE;
  printf("\tThank you. Now please send me a SIGCONT (`fg` on most shells. Or you can use `kill(1))`\n");

  /* raise the signal again so that the process is actually suspended */
  raise(SIGTSTP);
}

static void
continued(__attribute__((unused)) int signal) {
  /* UNSAFE: uses printf(3) */

  printf("\tHello from SIGCONT handler!\n");
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
