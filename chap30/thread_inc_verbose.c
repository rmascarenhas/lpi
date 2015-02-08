/* thread_inc_verbose.c - incorrectly increments a global variable on two threds.
 *
 * This creates two threads that both increment (i.e., read and write) a shared
 * variable without using any sychronization mechanism in order to demonstrate
 * the problems that the consequent race conditions generate. For large enough
 * values of the number of iterations to be performed (given on the command-line),
 * more the result will deviate from the expected amount.
 *
 * This program will output the current value of the global `glob` variable
 * prefixed by a thread identifier. The analysis of the output of this program
 * allows us to pinpoint when the kernel scheduler changed context to another
 * thread in the middle of the incrementing process, causing wrong results to
 * be generated.
 *
 * It is recommended to redirect the output of this program to a file to be able
 * to better analyze it later.
 *
 * Usage
 *
 *    $ ./thread_inc_verbose [num_its]
 *
 *    num_its - number of iterations each thread should perform. Defaults to NUM_INCS_DFL.
 *
 * Most of this program is based on Listing 30-1 of The Linux Programming Interface
 * book. Changes made by Renato Mascarenhas Costa.
 *
 * Obs:
 *
 * 	One way to determine points in which the `glob` variable was mistakenly overwritten
 * is using the following awk script (assuming the output of this program is in the
 * `out.txt` file):
 *
 * 	awk -F= '{ if ($2 < cur) printf("%d: %s\n", NR, $0); else cur = $2 }' <out.txt
 */

#ifndef NUM_INCS_DFL
#  define NUM_INCS_DFL (10000000)
#endif

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct threadinfo {
	int tid;
	long loops;
};

static void helpAndLeave(const char *progname, int status);
static void thread_pexit(int err, const char *fCall);

static void *threadFunc(void *arg);

static volatile long glob = 0;

int
main(int argc, char *argv[]) {
	if (argc > 2) {
		helpAndLeave(argv[0], EXIT_FAILURE);
	}

	pthread_t t1, t2;
	int s;
	long loops;
	struct threadinfo info1, info2;

	loops = (argc == 2 ? strtol(argv[1], NULL, 10) : NUM_INCS_DFL);
	info1.loops = info2.loops = loops;

	/* create both threads */
	info1.tid = 1;
	info2.tid = 2;
	s = pthread_create(&t1, NULL, threadFunc, &info1);
	if (s != 0) {
		thread_pexit(s, "pthread_create");
	}
	s = pthread_create(&t2, NULL, threadFunc, &info2);
	if (s != 0) {
		thread_pexit(s, "pthread_create");
	}

	/* join each of the threads */
	s = pthread_join(t1, NULL);
	if (s != 0) {
		thread_pexit(s, "pthread_join");
	}
	s = pthread_join(t2, NULL);
	if (s != 0) {
		thread_pexit(s, "pthread_join");
	}

	printf("glob = %ld\n", glob);
	exit(EXIT_SUCCESS);
}

static void *
threadFunc(void *arg) {
	struct threadinfo info = *((struct threadinfo *) arg);
	int loc;
	long j;

	for (j = 0; j < info.loops; ++j) {
		loc = glob;
		loc++;
		glob = loc;
		printf("[T%d] iteration #%ld - glob = %ld\n", info.tid, j+1, glob);
	}

	return NULL;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS) {
		stream = stdout;
	}

	fprintf(stream, "Usage: %s [num_its]\n", progname);
	exit(status);
}

static void
thread_pexit(int err, const char *fCall) {
	fprintf(stderr, "%s: %s\n", fCall, strerror(err));
	exit(EXIT_FAILURE);
}
