/* blocked_sigchld.c - shows effect of blocked SIGCHLD that has already been wait'ed.
 *
 * This program adds a handler for SIGCHLD and then blocks it. After this happens, a
 * child terminates and the parent performs a `wait(2)`, which should remove the
 * children from the process table.
 *
 * The behavior of the program shows us that, after the unblock of SIGCHLD, the blocked
 * signal *is* delivered to the process. This is important for the `system(3)`
 * function since it blocks SIGCHLD on the parent process. This behavior, then,
 * allows the caller to still have its handler run if it expects to do so on
 * `system(3)` calls.
 *
 * Usage
 *
 *    $ ./blocked_sigchld
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <sys/types.h>
#include <wait.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHILD_SLEEP_TIME (3)

static void handler(int signal);

static void logInfo(const char *who, const char *message);
static void pexit(const char *fCall);

int
main() {
  int status;
  sigset_t chld;
  struct sigaction st_sigchld;

  logInfo("Parent", "Adding handler and blocking SIGCHLD");

  /* add a handler for SIGCHLD... */
  st_sigchld.sa_handler = handler;
  st_sigchld.sa_flags = 0;
  if (sigaction(SIGCHLD, &st_sigchld, NULL) == -1) {
    pexit("sigaction");
  }

  /* ... then block it */
  sigemptyset(&chld);
  sigaddset(&chld, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &chld, NULL) == -1) {
    pexit("sigprocmask");
  }

  logInfo("Parent", "Creating child");

  switch (fork()) {
    case -1:
      pexit("fork");
      break;

    case 0:
      /* child: give parent a chance to wait - very simple means of process communication */
      logInfo("Child", "sleeping");
      sleep(CHILD_SLEEP_TIME);
      logInfo("Child", "finishing");
      _exit(EXIT_SUCCESS);
      break;
  }

  /* parent: falls through and wait for child */
  logInfo("Parent", "Waiting child");
  if (wait(&status) == -1) {
    pexit("wait");
  }

  /* now process table should not contain child anymore - unblock SIGCHLD */
  logInfo("Parent", "Unblocking SIGCHLD");
  if (sigprocmask(SIG_UNBLOCK, &chld, NULL) == -1) {
    pexit("sigprocmask");
  }

  exit(EXIT_SUCCESS);
}

static void
handler(int signal) {
  /* UNSAFE: printf(3) usage inside signal handler */
  printf("Got signal %s\n", strsignal(signal));
}

static void
logInfo(const char *who, const char *message) {
  char buf[BUFSIZ];
  int size;

  size = snprintf(buf, BUFSIZ, "[%s %ld] %s\n", who, (long) getpid(), message);
  if (write(STDOUT_FILENO, buf, size) == -1) {
    pexit("write");
  }
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
