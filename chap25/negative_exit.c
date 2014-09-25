/* negative_exit.c - show exit status seen by parent process when child calls exit(-1).
 *
 * The argument passed to the `exit(3)` library function indicates the termination
 * status of a process (at least the last 8 bits of it). It can be useful information
 * for the parent process to determine the reason for the child's termination, which
 * could have been normal or abnormal.
 *
 * This program demonstrates what happens when a child invokes `exit(3)` passing -1
 * as an argument. The exit status is presented by the parent using the `WEXITSTATUS`
 * macro, which interprets the significant 8 bits from a status returned by `wait(2)`.
 *
 * The answer depends on the hosts' architecture, but in general, the -1 value should
 * be represented in 1's complement, causing all of the last 8 bits to be set and as a
 * consequence the parent sees a 255 exit status value.
 *
 * Usage
 *
 *    $ ./negative_exit
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>

static void logInfo(const char *who, const char *message);

static void pexit(const char *fCall);

int
main() {
  int status;
  char buf[BUFSIZ];

  logInfo("Parent", "creating child");

  switch (fork()) {
    case -1:
      pexit("fork");
      break;

    case 0:
      /* child: invoke the `exit(3)` function with a negative value */
      logInfo("Child", "invoking exit(-1)");
      exit(-1);
      break; /* should not get here */

    default:
      /* parent: wait for child and inspect exit status */
      if (wait(&status) == -1) {
        pexit("wait");
      }

      snprintf(buf, BUFSIZ, "child has exited with status %d", WEXITSTATUS(status));
      logInfo("Parent", buf);
  }

  exit(EXIT_SUCCESS);
}

static void
logInfo(const char *who, const char *message) {
  char buf[BUFSIZ];
  int len;
  
  len = snprintf(buf, BUFSIZ, "[%s %ld] %s\n", who, (long) getpid(), message);
  if (write(STDOUT_FILENO, buf, len) == -1) {
    pexit("write");
  }
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
