/* threaded_operations.c - exercise the tsbintree implementation with multiple threads.
 *
 * The tsbintree is a thread-safe binary tree implementation. This test program aims to
 * exercise adding and removing elements from the tree from multiple threads at the same
 * time.
 *
 * The number of threads used can be set by defining the NUM_THREADS constant.
 *
 * Usage:
 *
 * 	$ ./test/threaded_operations
 *
 * Author: Renato Mascarenhas Costa
 */

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include "tsbintree.h"

#ifndef NUM_THREADS
#  define NUM_THREADS (100)
#endif

#define MAX_KEY_LEN (10)

static int DELTA = 1000;
static char VALUE[] = "TSBINTREE"; /* sample data to be used as value for every node */

struct thread_spec {
	tsbintree *tree;
	int tid;
	int start;
	int delta;
};

static void pexit(const char *fCall);
static void pthread_pexit(const int err, const char *fCall);

static int thread_spec_create(struct thread_spec **spec, int tid, tsbintree *tree, int start);
static void *add_nodes(void *spec);

int
main() {
	struct thread_spec *spec;
	tsbintree tree;
	int i, s, result, start;
	pthread_t *threads;
	void *r;

	tsbintree_init(&tree);

	threads = malloc(NUM_THREADS * sizeof(pthread_t));
	if (threads == NULL)
		pexit("malloc");

	printf(">>> Test suite for tsbintree starting.\nNUM_THREADS: %d\n", NUM_THREADS);

	spec = NULL;
	start = 0;
	for (i = 0; i < NUM_THREADS; ++i) {
		if (thread_spec_create(&spec, i+1, &tree, start) == -1)
			pexit("thread_spec_create");

		s = pthread_create(&threads[i], NULL, add_nodes, spec);
		if (s != 0)
			pthread_pexit(s, "pthread_create");

		start += DELTA;
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		s  = pthread_join(threads[i], &r);
		if (s != 0)
			pthread_pexit(s, "pthread_join");

		result = (int) r;
		if (result != 0) {
			printf("Thread %d failed with error %s\n", i+1, strerror(result));
			exit(EXIT_FAILURE);
		}
	}

#ifdef TSBT_DEBUG
	int n;
	n = tsbintree_print(&tree);
	printf("(%d elements)\n", n);
#else
	printf("Please enable debug support on tsbintree if you wish to see its contents.\n");
#endif

	free(threads);

	return EXIT_SUCCESS;
}

static int
thread_spec_create(struct thread_spec **spec, int tid, tsbintree *tree, int start) {
	*spec = malloc(sizeof(struct thread_spec));
	if (*spec == NULL)
		return -1;

	(*spec)->tree = tree;
	(*spec)->tid = tid;
	(*spec)->start = start;
	(*spec)->delta = DELTA;

	return 0;
}

static void *
add_nodes(void *arg) {
	struct thread_spec *spec = (struct thread_spec *) arg;
	int i, r;
	char *key;

	for (i = 0; i < spec->delta; ++i) {
		key = malloc(MAX_KEY_LEN);
		if (key == NULL)
			return (void *) -1;

		snprintf(key, MAX_KEY_LEN, "%d", spec->start + i);
		if (tsbintree_add(spec->tree, key, VALUE) == -1) {
			free(arg);
			r = errno;
			return (void *) r;
		}
		printf("#%d: %s\n", spec->tid, key);
	}

	free(arg);
	return (void *) 0;
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}

static void
pthread_pexit(const int err, const char *fCall) {
	errno = err;
	perror(fCall);
	exit(EXIT_FAILURE);
}
