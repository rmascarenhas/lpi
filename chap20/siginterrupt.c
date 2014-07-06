/* siginterrupt.c - implement siginterrupt(3) in terms of sigaction(2).
 *
 * The `siginterrupt(3)` library function is used to tell the kernel whether or
 * not it should restart blocking system calls when a signal is received. The caller
 * can either specify that system calls should be restarted after the signal handler
 * is finished (the default), or indicate that interrupted system calls should return
 * immediately with the EINTR error.
 *
 * This program implements the `siginterrupt(3)` library function using the `sigaction(2)`
 * system call, which is the current recommended way of performing such task, since
 * `siginterrupted(3)` is now deprecated. It accepts a command line argument that must
 * equal either 0 or 1, indicating that system calls should or should not be restarted,
 * respectively. After that, it waits for a number of seconds (defined by SIGINTERRUPT_SLEEPTIME)
 * and waits for the user to interrupt the program to verify that the behavior is
 * expected.
 *
 * Usage
 *
 *    $ ./siginterrupt <arg>
 *
 *    <arg> - either 0 or 1, to indicate that system calls should or should not
 *    be restarted after signals are handled.
 *
 * Author: Renato Mascarenhas Costa
 */

#ifndef SIGINTERRUPT_SLEEPTIME
#  define SIGINTERRUPT_SLEEPTIME (5)
#endif

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { FALSE, TRUE } Bool;

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static void handler(int sig);

static int _siginterrupt(int sig, int flag);

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  Bool flag;
  int status;
  struct sigaction action;

  if (!strncmp(argv[1], "0", 2)) {
    flag = FALSE;
  } else if (!strncmp(argv[1], "1", 2)) {
    flag = TRUE;
  } else {
    fprintf(stderr, "Invalid argument: %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  printf("Setting siginterrupt to %s\n", flag ? "true" : "false");
  if (_siginterrupt(SIGINT, flag) == -1) {
    pexit("_siginterrupt");
  }

  printf("Giving birth to a child process that will sleep for %d seconds, please interrupt me in this period\n", SIGINTERRUPT_SLEEPTIME);

  status = fork();

  if (status == -1) {
    pexit("fork");
  } else if (status == 0) {
    /* ignore interruptions */
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGINT, &action, NULL) == -1) {
      pexit("sigaction");
    }

    sleep(SIGINTERRUPT_SLEEPTIME);
  } else {
    /* setup SIGINT handler */
    action.sa_handler = handler;
    if (sigaction(SIGINT, &action, NULL) == -1) {
      pexit("sigaction");
    }

    errno = 0;
    if (wait(&status) == -1) {
      if (errno == EINTR) {
        printf("wait(2) returned -1 with errno set to EINTR\n");
      } else {
        pexit("wait");
      }
    } else {
      printf("wait(2) call finished successfully\n");
    }
  }

  exit(EXIT_SUCCESS);
}

static int
_siginterrupt(int signal, int flag) {
  struct sigaction actbuff;

  if (sigaction(signal, NULL, &actbuff) == -1) {
    return -1;
  }

  if (flag) {
    actbuff.sa_flags &= ~SA_RESTART;
  } else {
    actbuff.sa_flags &= SA_RESTART;
  }

  if (sigaction(signal, &actbuff, NULL) == -1) {
    return -1;
  }

  return 0;
}

static void
handler(__attribute((unused)) int signal) {
  printf("\tInterrupt signal received, nothing to be done\n");
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <0|1>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
