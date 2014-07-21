/* abort.c - implements the abort(3) library function.
 *
 * The `abort(3)` function can be used to termiate a program. It does so by triggering
 * a SIGABRT signal to the calling process, whose default action is to terminate
 * the process generating a core dump. SUSv3 specifies that the `abort` function
 * must terminate the process for every program that has a handler that returns.
 *
 * This program is an implementation of the `abort(3)` function. It takes one
 * command line argument that indicates how it should behave:
 *
 *  0 - no handler is installed for the SIGABRT signal, causing the default
 *      disposition to be applied.
 *
 *  1 - causes SIGABRT to be ignored.
 *
 *  2 - a returning handler is installed.
 *
 *  3 - a non-returning handler is installed. `abort` should not terminate
 *      the process in this situation.
 *
 * Usage
 *
 *    $ ./abort <arg>
 *
 *    arg - tell the program what to do. Refer to the possibilities above.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE

#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void _abort(void);
static void abortHandler(int sig);

static volatile sig_atomic_t performJump = 0;
static sigjmp_buf env;

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  struct sigaction act;

  if (!strncmp(argv[1], "0", 2)) {
    /* do not install a signal handler for SIGABRT */
    printf("No handler installed, aborting\n");
    _abort();
  } else if (!strncmp(argv[1], "1", 2)) {
    printf("Ignoring SIGABRT and aborting\n");

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGABRT);
    act.sa_handler = SIG_IGN;

    if (sigaction(SIGABRT, &act, NULL) == -1) {
      pexit("sigaction\n");
    }

    _abort();
  } else if (!strncmp(argv[1], "2", 2) || !strncmp(argv[1], "3", 2)) {
    /* install a handler for SIGABRT */

    if (!strncmp(argv[1], "2", 2)) {
      printf("Installing a returning handler and aborting\n");
    } else {
      printf("Installing a non-returning handler and aborting\n");
      performJump = 1;
    }

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGABRT);
    act.sa_handler = abortHandler;

    if (sigaction(SIGABRT, &act, NULL) == -1) {
      pexit("sigaction");
    }

    if (sigsetjmp(env, 1) == 0) {
      _abort();
    } else {
      /* non-returning handler will jump to this point */
      printf("Jump performed to main function\n");
    }
  } else {
    fprintf(stderr, "Invalid argument: %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  printf("Finishing execution successfully\n");
  exit(EXIT_SUCCESS);
}

static void
_abort() {
  sigset_t sigabrt;
  struct sigaction oldHandler;

  /* first of all, unblock SIGABRT */
  sigemptyset(&sigabrt);
  sigaddset(&sigabrt, SIGABRT);

  if (sigprocmask(SIG_UNBLOCK, &sigabrt, NULL) == -1) {
    pexit("sigprocmask");
  }

  if (sigaction(SIGABRT, NULL, &oldHandler) == -1) {
    pexit("sigaction");
  }

  if (oldHandler.sa_handler == SIG_IGN) {
    /* program was set to ignore SIGABRT. However, the program should still be
     * terminated when `abort` is called, so we send the signal anyway */
    oldHandler.sa_handler = SIG_DFL;
    if (sigaction(SIGABRT, &oldHandler, NULL) == -1) {
      pexit("sigaction");
    }
  }

  if (oldHandler.sa_handler != SIG_DFL) {
    /* a custom handler for SIGABRT was specified. We just send the signal and,
     * in case the handler returns, we just remove the handler and re-raise the
     * signal, after performing cleanup. If the handler does not return, then
     * execution continues normally. */
    raise(SIGABRT);

    /* if we get to this point, the handler returned */

    /* add default behavior to SIGABRT so that the process can terminate */
    oldHandler.sa_handler = SIG_DFL;
    if (sigaction(SIGABRT, &oldHandler, NULL) == -1) {
      pexit("sigaction");
    }
  }

  /* flush stdio streams */
  if (fflush(stdout) == EOF) {
    pexit("fflush");
  }

  if (fflush(stderr) == EOF) {
    pexit("fflush");
  }

  /* close streams */
  if (close(STDOUT_FILENO) == -1) {
    pexit("close");
  }

  if (close(STDERR_FILENO) == -1) {
    pexit("close");
  }

  /* raise SIGABRT again, this time sure that the process will be terminated */
  raise(SIGABRT);
}

static void
abortHandler(__attribute__((unused)) int signal) {
  printf("\tHandler for SIGABRT invoked\n");

  if (performJump) {
    printf("Jumping to main function\n");
    siglongjmp(env, 1);
  }
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <0|1|2|3>\n", progname);
  fprintf(stream, "\t0 - no handler is installed for SIGABRT\n");
  fprintf(stream, "\t1 - causes SIGABRT to be ignored\n");
  fprintf(stream, "\t2 - a returning handler is installed\n");
  fprintf(stream, "\t3 - a non-returning handler is installed\n");
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
