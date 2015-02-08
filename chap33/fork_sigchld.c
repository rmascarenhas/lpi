/* fork_sigchld.c - shows which thread handles SIGCHLD if a thread executes fork(2).
 *
 * Signal dispositions are inherited when new threads are created. If a signal has
 * a custom handler, then the POSIX Threads standard specifies that an arbitrary
 * thread will handle the asynchronous signal.
 *
 * This program demonstrates that that is the case by having a thread perform a
 * fork(2) system call and then shows that, upon child termination, a different
 * thread may handle the resulting SIGCHLD.
 *
 * Usage
 *
 *    $ ./fork_sigchld
 *
 * Author: Renato Mascarenhas Costa
 */

#ifndef NUMTHREADS
#  define NUMTHREADS (256)
#endif

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fork_done = 0;

static void pexit(const char *fCall);
static void pthread_pexit(int err, const char *fCall);

static void chld_handler(int sig);
static void *fork_or_wait(void *arg);

int
main() {
	pthread_t threads[NUMTHREADS];
	struct sigaction act;
	sigset_t chldset;
	int i, s, forking_thread, do_fork;

	sigemptyset(&chldset);
	sigaddset(&chldset, SIGCHLD);

	act.sa_handler = chld_handler;
	act.sa_mask = chldset;

	if (sigaction(SIGCHLD, &act, NULL) == -1)
		pexit("sigaction");

	/* at each time, an aribitrarily chosen thread will be the one to
	 * actually perform the forking */
	srand(time(NULL));
	forking_thread = rand() % NUMTHREADS;

	/* create threads */
	for (i = 0; i < NUMTHREADS; ++i) {
		do_fork = (i == forking_thread);
		s = pthread_create(&threads[i], NULL, fork_or_wait, &do_fork);
		if (s != 0)
			pthread_pexit(s, "pthread_create");
	}

	/* wait for threads to terminate */
	for (i = 0; i < NUMTHREADS; ++i) {
		s = pthread_join(threads[i], NULL);
		if (s != 0)
			pthread_pexit(s, "pthread_join");
	}

	printf("Main thread: finishing\n");
	exit(EXIT_SUCCESS);
}

static void *
fork_or_wait(void *arg) {
	int do_fork = *((int *) arg);
	pthread_t curr_thread;

	if (do_fork) {
		switch (fork()) {
			case -1:
				pexit("fork");
				break;

			case 0:
				/* child: just terminate */
				_exit(EXIT_SUCCESS);
				break;

			default:
				/* parent: identify itself and indicate that fork
				 * has been done */
				curr_thread = pthread_self();
				printf("Performing fork on thread at %p\n", &curr_thread);
				fork_done = 1;
				break;
		}
	} else {
		/* wait for fork to be done */
		while (!fork_done)
			sleep(1);

		/* pretend to do some work so that all threads are eligible to
		 * respond to SIGCHLD when the child is terminated */
		sleep(2);
	}

	return NULL;
}

static void
chld_handler(int sig) {
	pthread_t curr_thread;
	curr_thread = pthread_self();

	/* not async-safe function! */
	printf("\t Handling signal %d (%s) by thread at %p\n", sig, strsignal(sig), &curr_thread);
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
