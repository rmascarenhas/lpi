/* dead_parent_init.c - shows that when the parent dies, `init` inherits the process.
 *
 * When a process creates a child, calls to `getppid(2)` will correctly return the
 * PID for the parent process. However, if the parent dies while the child is still
 * running, then the `init` process becomes the parnent of the orphan child and
 * `getppid(2)` reflects this change.
 *
 * This program demonstrates the behavior described above, by creating a child process
 * and inspecting the return of the `getppid(2)` system call after the parent is
 * finished.
 *
 * Usage
 *
 *    $ ./dead_parent_init
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void logInfo(const char *who, const char *message);

static void pexit(const char *fCall);

int
main() {
  logInfo("Parent", "creating child");

  switch (fork()) {
    case -1:
      pexit("fork");
      break;

    case 0:
      /* child code: give a chance for parent finish itself (very basic means of
       * process synchronization */
      logInfo("Child", "child created, waiting parent to finish");
      sleep(5);

      char msg[BUFSIZ];
      snprintf(msg, BUFSIZ, "parent PID = %ld", (long) getppid());

      logInfo("Child", msg);
      break;

    default:
      /* parent: nothing to do but finish */
      logInfo("Parent", "finishing up");
      exit(EXIT_SUCCESS);
  }

  /* only child should get to this point */
  exit(EXIT_SUCCESS);
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
