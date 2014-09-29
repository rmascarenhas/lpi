/* zombie_parent.c - shows what happens when the parent of a process becomes a zombie.
 *
 * When a child process finishes, most resources it used are freed and become available
 * for other processes. However, information about the process itself such as the PID
 * number, termination status and resource usage statistics are still kept in the kernel's
 * data structures since it must be available to the parent process in case it performs
 * one of the `wait(2)` family of functions.
 *
 * This program shows what the kernel reports as being the parent of a process when the
 * parent becomes a zombie. Does it show the zombie PID or it already returns the ID
 * of the init process?
 *
 * The answer is - zombie process are *ignored*. The child process will report its parent
 * as being init even though the original parent exists as a zombie.
 *
 * Usage
 *
 *    $ ./zombie_parent
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
  logInfo("Grandparent", "creating parent");

  switch (fork()) {
    case -1:
      pexit("fork");
      break;

    case 0:
      /* parent code: create child and then finish in order to become a zombie */
      logInfo("Parent", "parent created, creating child");
      switch (fork()) {
        case -1:
          pexit("fork");
          break;

        case 0:
          /* child: wait for parent to terminate and then inspect its parent PID */
          logInfo("Child", "child created, waiting for parent to terminate");
          sleep(2);

          char msg[BUFSIZ];
          snprintf(msg, BUFSIZ, "parent PID = %ld", (long) getppid());

          logInfo("Child", msg);
          break;

        default:
          /* parent created child - just finish */
          logInfo("Parent", "terminating - and consequently becoming a zombie");
          exit(EXIT_SUCCESS);
      }
      break;

    default:
      /* grandparent: just wait for everything to finish - very simple
       * means of achieving so. */
      logInfo("Grandparent", "waiting for everything to be done");
      sleep(5);
  }

  /* child and grandparent should terminate here */
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
