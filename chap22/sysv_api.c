/* sysv_api.c - implements the obsolete System V signal API using POSIX.
 *
 * Previously, both System V and BSD had its own signal API. POSIX defined a
 * standard API for handling signals on UNIX systems which defined a more powerful
 * way of changing signal dispositions, restarting interrupted system calls,
 * among other improvements.
 *
 * This program implements the - now obsolete - System V API using standard POSIX
 * signal API. It accepts a numeric command line argument that tells it what
 * to do. Each number tests one or more functions from the System V API. Refer to
 * the usage guide for information on the available options.
 *
 * Usage
 *
 *    $ ./sysv_api <numop>
 *    numop - the operation number that defines the program behavior. Possible values are:
 *
 *      1 - sigset    - Changes SIGINT disposition.
 *      2 - sighold   - Blocks SIGINT.
 *      3 - sigrelse  - Blocks SIGINT and then removes it from the process signal procmask.
 *      4 - sigignore - Igores SIGINT.
 *      5 - sigpause  - suspends execution until SIGINT is received.
 *
 * All operations will wait for a SIGINT before finishing so that the function
 * implementation can be verified.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 700

#include <limits.h>
#ifndef LONG_MIN
#  include <linux/limits.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*sighandler_t)(int);

enum { SIGSET = 1, SIGHOLD = 2, SIGRELSE = 3, SIGIGNORE = 4, SIGPAUSE = 5 };

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void handler(int sig);

static sighandler_t _sigset(int sig, sighandler_t handler);
static int _sighold(int sig);
static int _sigrelse(int sig);
static int _sigignore(int sig);
static int _sigpause(int sig);

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  char *endptr;
  long operation;
  struct sigaction action;

  operation = strtol(argv[1], &endptr, 10);
  if (operation == LONG_MIN || operation == LONG_MAX || endptr == argv[1] || operation < 1 || operation > 4) {
    fprintf(stderr, "%s: invalid operation %s\n", argv[0], argv[1]);
    exit(EXIT_FAILURE);
  }

  switch (operation) {
    case SIGSET:
      if (_sigset(SIGINT, handler) == (sighandler_t) -1) {
        pexit("_sigset");
      }

      printf("Changed disposition of SIGSET. Type Ctrl-C to see its handler.\n");
      pause();

      break;
    case SIGHOLD:
      if (_sighold(SIGINT) == -1) {
        pexit("_sighold");
      }

      printf("SIGINT is blocked. Ctrl-C will not stop this process.\n");
      pause();
      break;

    case SIGRELSE:
      if (_sighold(SIGINT) == -1) {
        pexit("_sighold");
      }
      printf("SIGINT blocked\n");

      if (_sigrelse(SIGINT) == -1) {
        pexit("_sigrelse");
      }
      printf("SIGINT unblocked. You can now finish this process with Ctrl-C\n");
      pause();
      break;

    case SIGIGNORE:
      if (_sigignore(SIGINT) == -1) {
        pexit("_sigignore");
      }
      printf("SIGINT is ignored. Ctrl-C will not stop this process\n");
      pause();
      break;

    case SIGPAUSE:
      action.sa_handler = handler;

      if (sigaction(SIGINT, &action, NULL) == -1) {
        pexit("sigaction");
      }

      printf("Suspending process until you hit Ctrl-C...\n");
      _sigpause(SIGINT);
      printf("Execution is back\n");
      break;
  }

  exit(EXIT_SUCCESS);
}

static sighandler_t
_sigset(int sig, sighandler_t handler) {
  sigset_t blocked_signals;
  struct sigaction act, oldact;

  if (sigprocmask(SIG_BLOCK, NULL, &blocked_signals) == -1) {
    return (sighandler_t) -1;
  }

  act.sa_handler = handler;
  if (sigaction(sig, &act, &oldact) == -1) {
    return (sighandler_t) -1;
  }

  if (handler == SIG_HOLD) {
    _sighold(sig);
    return oldact.sa_handler;
  }

  if (sigismember(&blocked_signals, sig)) {
    return SIG_HOLD;
  } else {
    return oldact.sa_handler;
  }
}

static int
_sighold(int sig) {
  sigset_t block_mask;

  sigemptyset(&block_mask);
  sigaddset(&block_mask, sig);

  return sigprocmask(SIG_BLOCK, &block_mask, NULL);
}

static int
_sigrelse(int sig) {
  sigset_t unblock_mask;

  sigemptyset(&unblock_mask);
  sigaddset(&unblock_mask, sig);

  return sigprocmask(SIG_UNBLOCK, &unblock_mask, NULL);
}

static int
_sigignore(int sig) {
  struct sigaction action;

  action.sa_handler = SIG_IGN;
  return sigaction(sig, &action, NULL);
}

static int
_sigpause(int sig) {
  sigset_t mask;
  if (sigprocmask(SIG_SETMASK, NULL, &mask) == -1) {
    pexit("sigprocmask");
  }

  sigdelset(&mask, sig);

  return sigsuspend(&mask);
}

static void
handler(__attribute__((unused)) int sig) {
  /* UNSAFE: uses printf(3) */
  printf("\tSIGINT received\n");
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <numop>\n", progname);
  fprintf(stream, "\t1 - sighold   - Blocks SIGINT\n");
  fprintf(stream, "\t2 - sigrelse  - Blocks SIGINT and then removes it from the process procmask\n");
  fprintf(stream, "\t3 - sigignore - Ignores SIGINT\n");
  fprintf(stream, "\t4 - sigpause  - suspends execution until SIGINT is received\n");

  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
