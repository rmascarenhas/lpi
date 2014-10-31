/* one_time_init.c - implements a clone of the pthread_once(3) function.
 *
 * The pthread_once(3) function allows a multithreaded application to perform
 * one-time initialization. The thread function can call the pthread_once(3) function
 * passing a pointer to the initialization function, and the environment ensures
 * that only the first thread that reaches that point will execute the initialization
 * function. All other threads will just skip it.
 *
 * This program implements a clone of that function named one_time_init. As far as the
 * implementation goes, it receives a control and a function pointer argument, just
 * like pthread_once(3) does. The control paramenter is a statically allocated struct
 * comprised by a Boolean attribute indicating whether or not the initialization
 * already happened and a mutex to control access to this shared variable.
 *
 * This program creates multiple threads to execute the same function that has
 * an initialization step within it. By default, the number of threads created
 * is 10. This number can be overwritten by defining NUM_THREADS on compilation
 * time.
 *
 * Usage
 *
 *    $ ./one_time_init
 *
 * Author: Renato Mascarenhas Costa
 */

#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef NUM_THREADS
#  define NUM_THREADS (10)
#endif

#define ONE_TIME_INIT_VALUE (42)

#define INIT_CONTROL_INITIALIZER { FALSE, PTHREAD_MUTEX_INITIALIZER }

enum Boolean { FALSE, TRUE };

struct init_control {
	enum Boolean initialized;
	pthread_mutex_t lock;
};
static int one_time_init(struct init_control *control, void (*init_routine)(void));


/* this value should never be read as -10 in the threads since it will be
 * initialized once to a different value */
int to_be_initialized = -10;
static struct init_control control = INIT_CONTROL_INITIALIZER;

static void init_function();
static void *thread_function(void *arg);

static void pexit(const char *fCall);
static void pthread_pexit(int err, const char *fCall);

int
main() {
	int s, i;
	void *res;
	int tids[NUM_THREADS];
	pthread_t threads[NUM_THREADS];

	printf("Main thread: creating %d threads\n", NUM_THREADS);
	for (i = 0; i < NUM_THREADS; ++i) {
		tids[i] = i + 1;
		s = pthread_create(&threads[i], NULL, thread_function, &tids[i]);
		if (s != 0)
			pthread_pexit(s, "pthread_create");
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		s = pthread_join(threads[i], &res);
		if (s != 0)
			pthread_pexit(s, "pthread_join");
	}

	printf("Main thread: all threads finished, terminating\n");
	exit(EXIT_SUCCESS);
}

static void *
thread_function(void *arg) {
	int tid = *((int *) arg);
	if (one_time_init(&control, init_function) == -1)
		pexit("one_time_init");

	printf("%d: value of shared variable: %d\n", tid, to_be_initialized);
	return NULL;
}

static void
init_function(void) {
	printf("Performing initialization\n");
	to_be_initialized = ONE_TIME_INIT_VALUE;
}

static int
one_time_init(struct init_control *control, void (*init_function)(void)) {
	if (!control) {
		errno = EINVAL;
		return -1;
	}

	int s;

	/* reading the value of the boolean value requires mutex protection */
	s = pthread_mutex_lock(&control->lock);
	if (s != 0)
		pthread_pexit(s, "pthread_mutex_lock");

	if (!control->initialized) {
		(*init_function)();
		control->initialized = TRUE;
	}

	s = pthread_mutex_unlock(&control->lock);
	if (s != 0)
		pthread_pexit(s, "pthread_mutex_unlock");

	return 0;
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
