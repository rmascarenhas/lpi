/* vfork_fd.c - shows that changing file descriptor in a child created with vfork(2)
 *              does not affect parent.
 *
 * The `vfork(2)` system call was added by BSD implementations in order to speed up the
 * process of creating child processes. In earlier implementations, a call to `fork(2)`
 * would immediately copy all the stack and heap of the parent process to the child, an
 * effort that could be wasted in case the child was created to perform an `exec()` call.
 * To improve this scenario, the `vfork(2)` call was added: whenever it is called, the parent
 * is stopped until the point when the child successfully performs an `exec` or `_exit`
 * call.
 *
 * Due to this rather strange semantics and to many possible undefined behaviors that it can
 * generate (as the child will share the same address space as the parent, changes on the heap and
 * stack are reflected on the parent when it is scheduled again), this function is kept
 * only for portability purposes. As the `fork(2)` call currently addresses such performance
 * issues, using `vfork(2)` is not advised and was removed from SUSv4.
 *
 * This program demonstrates that, contrary to memory space, file descriptors changes
 * in the child are *not* reflected in the parent.
 *
 * Usage
 *
 *    $ ./vfork_fd
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE

#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void logInfo(const char *who, const char *message);

static void pexit(const char *fCall);

int
main() {
  logInfo("Parent", "Calling vfork(2)");

  switch (vfork()) {
    case -1:
      pexit("vfork");
      break;

    case 0:
      /* child closes STDOUT and finishes */
      logInfo("Child", "Closing stdout and finishing");

      if (close(STDOUT_FILENO) == -1) {
        pexit("close");
      }

      _exit(0);

      break;

    default:
      /* parent - the code here will only be executed after the child is done - in this
       * case, called _exit(2) */
      logInfo("Parent", "Back. If you can read this is because the file descriptor was unaffected");

      break;
  }

  exit(EXIT_SUCCESS);
}

static void
logInfo(const char *who, const char *message) {
  char str[BUFSIZ];
  int size;

  size = snprintf(str, BUFSIZ, "[%s %ld] %s\n", who, (long) getpid(), message);
  if (write(STDOUT_FILENO, str, size) == -1) {
    pexit("write");
  }
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
