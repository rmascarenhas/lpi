/* resethand.c - demonstrates the effect of the SA_RESETHAND flag of sigaction(2).
 *
 * The SA_RESETHAND is a flag that can be passed to the `sigaction(2)` system call
 * and tells the kernel to reset the signal disposition before executing the handler.
 * This has the effect of registering the handler just for the next signal of the
 * given type that arrives (thus the legacy name SA_ONESHOT).
 *
 * This program verifies this behavior by registering a custom handler for the
 * SIGINT signal, printing a message in the screen. The second time the signal
 * is sent, the default action is taken and the process is terminated.
 *
 * Usage
 *
 *    $ ./resethand
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pexit(const char *fCall);
static void handler(int sig);

int
main() {
  struct sigaction action;

  action.sa_handler = handler;
  action.sa_flags = SA_RESETHAND;

  if (sigaction(SIGINT, &action, NULL) == -1) {
    pexit("sigaction");
  }

  printf("Hit Control-C, please\n");

  /* wait for signals indefinitely */
  for (;;) {
    pause();
  }

  /* should never get to this point */
  exit(EXIT_SUCCESS);
}

static void
handler(__attribute__((unused)) int signal) {
  printf("\tHello, I am a custom handler registered with SA_RESETHAND. Hit Control-C again to finish execution\n");
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
