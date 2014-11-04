/* fib_cancel.c - demonstrates the thread cancellation concept.
 *
 * Threads whose calculation is no longer needed can be canceled. A thread can
 * customize its behavior in case it is sent a cancelation request. By default,
 * threads can be canceled, and cancelation is deferred, i.e., happens when a
 * function in a set specified by SUSv3 is invoked.
 *
 * These functions tend to perform operations that may block due to external
 * dependencies or devices. In case a thread function is performing a CPU
 * bound activity, then it might never actually be canceled. One way to work
 * around this situation is via the pthread_testcancel(3) function, which
 * cancels a thread immediately in case there is a pending cancelation request.
 *
 * This program exemplifies the usage of the pthread_testcancel(3) function
 * by creating a thread that calculates the nth Fibonacci number (a purely
 * computational task that could not be canceled automatically) and calls
 * pthread_testcancel(3) after some iterations to ensure the computation
 * can be canceled. If the program is compiled with the NOTESTCANCEL symbol
 * defined, then cancelation test is not performed, and the Fibonacci thread
 * will not be canceled even if a request is sent.
 *
 * Usage
 *
 *    $ ./fib_cancel <n>
 *
 *    n - makes the program create a thread that calculates the nth Fibonacci number.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);
static void pthread_pexit(int err, const char *fCall);

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  over = PTHREAD_COND_INITIALIZER;
static pthread_t worker, ui;

static void *calculate_fib(void *arg);
static void *ask_cancelation(void *arg);

int
main(int argc, char *argv[]) {
	if (argc != 2)
		helpAndLeave(argv[0], EXIT_FAILURE);

	long n;
	n = strtol(argv[1], NULL, 10);

	if (n < 0)
		helpAndLeave(argv[0], EXIT_FAILURE);

	int s;
	void *res;

	s = pthread_create(&ui, NULL, ask_cancelation, NULL);
	if (s != 0)
		pthread_pexit(s, "pthread_create");

	printf("Main thread: creating worker thread\n");
	s = pthread_create(&worker, NULL, calculate_fib, &n);
	if (s != 0)
		pthread_pexit(s, "pthread_create");

	s = pthread_cond_wait(&over, &lock);
	if (s != 0)
		pthread_pexit(s, "pthread_cond_wait");

	s = pthread_join(worker, &res);
	if (s != 0)
		pthread_pexit(s, "pthread_join");

	if (res == PTHREAD_CANCELED)
		printf("Thread was canceled.\n");
	else
		printf("Thread returned. Calculation result: %lld\n", *((unsigned long long *) res));

	exit(EXIT_SUCCESS);
}

static void *
ask_cancelation(__attribute__((unused)) void *arg) {
	printf("Press any key to cancel computation\n");
	getchar();

	printf("Sending cancelation request\n");
	int s;
	s = pthread_cancel(worker);
	if (s != 0)
		pthread_pexit(s, "pthread_cancel");

	s = pthread_cond_signal(&over);
	if (s != 0)
		pthread_pexit(s, "pthread_cond_signal");

	return NULL;
}

static void *
calculate_fib(void *arg) {
	long n = *((long *) arg);
	int i, s, f0, f1;
	unsigned long long *res;

	res = malloc(sizeof(unsigned long long));
	if (!res)
		pexit("malloc");

	f0 = 0; f1 = 1;
	if (n == 0) {
		*res = f0;
	} else if (n == 1) {
		*res = f1;
	} else {
		for (i = 2; i <= n; ++i) {
			*res = f0 + f1;
			f0 = f1;
			f1 = *res;

			/* test for a cancelation request */
#ifndef NOTESTCANCEL
			pthread_testcancel();
#endif
		}
	}

	/* result is done, wake up main thread */
	s = pthread_cond_signal(&over);
	if (s != 0)
		pthread_pexit(s, "pthread_cond_signal");

	return res;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s <n>\n", progname);
	exit(status);
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
