/* join_self.c - verifies the effect of joining itself on the host platform.
 *
 * Using POSIX threads, a thread can join with another using `pthread_join(3)`.
 * This function expects the thread identifier (TID) to be waited for. This
 * program demonstrates what happens if a thread issues a `pthread_join(3)`
 * using its own thread identifier.
 *
 * Reading the manual pages of the `pthread_join(3)` function on Linux, it is
 * already known that EDEADLK is issued.
 *
 * Usage
 *
 *    $ ./join_self
 *
 * Author: Renato Mascarenhas Costa
 */

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void thread_pexit(int err, const char *fCall);

int
main() {
  int s;
  if ((s = pthread_join(pthread_self(), NULL)) != 0) {
    thread_pexit(s, "pthread_join");
  }

  printf("Successfully joined with itself.\n");
  exit(EXIT_SUCCESS);
}

static void
thread_pexit(int err, const char *fCall) {
  fprintf(stderr, "%s: %s\n", fCall, strerror(err));
  exit(EXIT_FAILURE);
}
