/* thread_pending_signals.c - demonstrates that threads different pending signal queues.
 *
 * POSIX Threads specify that different threads within the same process must have
 * different pending signals queues. This program demonstrates that is the case
 * in the running NPTL implementation by creating two different threads and then
 * sending them signals using the pthread_kill(3) function.
 *
 * Usage
 *
 *    $ ./thread_pending_signals
 *
 * Author: Renato Mascarenhas Costa
 */

#define _GNU_SOURCE

#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int signalsent = 0;
static pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

static void pexit(const char *fCall);
static void pthread_pexit(int err, const char *fCall);
static void *print_pending_signals(void *arg);

int
main() {
	sigset_t blockset, oldset;
	int s;
	pthread_t t1, t2;
	int tid1, tid2;

	/* block SIGINT and SIGTERM in the main thread so that the created threads
	 * will inherit the signal mask */
	sigemptyset(&blockset);
	sigaddset(&blockset, SIGINT);
	sigaddset(&blockset, SIGTERM);
	s = pthread_sigmask(SIG_BLOCK, &blockset, &oldset);
	if (s != 0)
		pthread_pexit(s, "pthread_sigmask");

	/* create both threads */
	tid1 = 1;
	s = pthread_create(&t1, NULL, print_pending_signals, &tid1);
	if (s != 0)
		pthread_pexit(s, "pthread_create");

	tid2 = 2;
	s = pthread_create(&t2, NULL, print_pending_signals, &tid2);
	if (s != 0)
		pthread_pexit(s, "pthread_create");

	/* both threads are waiting for signals to be sent */
	s = pthread_kill(t1, SIGINT);
	if (s != 0)
		pthread_pexit(s, "pthread_kill");

	s = pthread_kill(t2, SIGTERM);
	if (s != 0)
		pthread_pexit(s, "pthread_kill");

	/* alert threads that signals are sent */
	printf("Main thread: signals were sent\n");
	signalsent = 1;

	/* join both threads and terminate */
	s = pthread_join(t1, NULL);
	if (s != 0)
		pthread_pexit(s, "pthread_join");

	s = pthread_join(t2, NULL);
	if (s != 0)
		pthread_pexit(s, "pthread_join");

	s = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
	if (s != 0)
		pthread_pexit(s, "pthread_sigmask");

	exit(EXIT_SUCCESS);
}

static void *
print_pending_signals(__attribute__((unused)) void *arg) {
	int tid = *((int *) arg);

	/* wait for main thread to send signals */
	while (!signalsent)
		sleep(1);

	/* print signal mask */
	sigset_t mask;
	int total = 0, sig, s;

	if (sigpending(&mask) == -1)
		pexit("sigpending");

	/* avoid mixing output from both threads */
	s = pthread_mutex_lock(&print_lock);
	if (s != 0)
		pthread_pexit(s, "pthread_mutex_lock");

	printf("Thread %d signal mask: ", tid);
	for (sig = 1; sig < NSIG; ++sig) {
		if (sigismember(&mask, sig)) {
			++total;
			printf("%s ", strsignal(sig));
		}
	}

	if (total == 0)
		printf("<empty>");

	printf("\n");

	s = pthread_mutex_unlock(&print_lock);
	if (s != 0)
		pthread_pexit(s, "pthread_mutex_unlock");

	return NULL;
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}

static void
pthread_pexit(int err, const char *fCall) {
	errno = err;
	pexit(fCall);
}
