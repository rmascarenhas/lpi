/* nodefer.c - demonstrates the effect of the SA_NODEFER flag of sigaction(2).
 *
 * The SA_NODEFER is a flag that can be passed to the `sigaction(2)` system call
 * and tells the kernel not to block the received signal while the handler is
 * running. As a consequence, if the same signal is received during the handler
 * execution, the handler itself will be suspended and another handler instance
 * will start running.
 *
 * This program demonstrates this behavior by registering a handler for the
 * SIGINT signal that prints a message and then sleeps. The user is able to
 * confirm that another instance of the handler is executed by triggering another
 * interrupt while the first one is sleeping. It is also possible to notice
 * that the first handler will only finish after the second one is done.
 *
 * Usage
 *
 *    $ ./nodefer
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NODEFER_SLEEPTIME
#  define NODEFER_SLEEPTIME (5)
#endif

static void pexit(const char *fCall);
static void handler(int sig);

int
main() {
  struct sigaction action;

  action.sa_handler = handler;
  action.sa_flags = SA_NODEFER;

  if (sigaction(SIGINT, &action, NULL) == -1) {
    pexit("sigaction");
  }

  printf("Hit Control-C, please (program will terminate after 10 interrupts)\n");

  /* wait for signals indefinitely */
  for (;;) {
    pause();
  }

  /* should never get to this point */
  exit(EXIT_SUCCESS);
}

static void
handler(__attribute__((unused)) int signal) {
  static int count = 0;

  ++count;
  if (count == 10) {
    exit(EXIT_SUCCESS);
  }

  int id = count;
  printf("\t[%d] SIGINT received. Sleeping for %d seconds...\n", id, NODEFER_SLEEPTIME);
  sleep(NODEFER_SLEEPTIME);

  printf("\t[%d] Done\n", id);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
