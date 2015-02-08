/* dirname_r.c - thread-safe implementation of the dirname(3) function.
 *
 * The dirname(3) function returns the name of the directory of a file, i.e.,
 * all the components of the path prior to the last (non-leading) slash '/'
 * character. See the man page for further explanation and edge cases.
 *
 * In Linux, the dirname(3) function is already thread-safe. However, SUSv3
 * does not require it to be thread-safe, allowing a compliant implementation
 * to return a statically allocated buffer that could be overwritten in a
 * multithreaded application.
 *
 * This program implements the function in a reentrant manner, using the thread-
 * specific data API.
 *
 * Usage
 *
 *    $ ./dirname_r <main-thread-path> <thread1-path> [...]
 *
 *    At least two paths are necessary for this program. The first one has the
 *    basename calculated by the main thread; the second one by a second thread,
 *    and so on. In order to test reentrancy, at least two paths must be given,
 *    but the user is free to pass as many paths as desired.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <limits.h>
#ifndef PATH_MAX
#  include <linux/limits.h>
#endif

#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);
static void pthread_pexit(int err, const char *fCall);

static char *dirname_r(char *path);

static void *thread_function(void *arg);

struct thread_info {
	int tid;
	char *path;
};

int
main(int argc, char *argv[]) {
	if (argc < 3)
		helpAndLeave(argv[0], EXIT_FAILURE);

	char *main_thread_dirname;
	int num_threads = argc - 2; /* discard program name and main thread path */
	int s, i;
	struct thread_info thread_infos[num_threads];
	pthread_t *threads;
	void *res;

	main_thread_dirname = dirname_r(argv[1]);

	threads = malloc(num_threads * sizeof(pthread_t));
	if (!threads)
		pexit("malloc");

	for (i = 0; i < num_threads; ++i) {
		thread_infos[i].tid = i + 1;
		thread_infos[i].path = argv[i + 2];

		s = pthread_create(&threads[i], NULL, thread_function, &thread_infos[i]);
		if (s != 0)
			pthread_pexit(s, "pthread_create");
	}

	for (i = 0; i < num_threads; ++i) {
		s = pthread_join(threads[i], &res);
		if (s != 0)
			pthread_pexit(s, "pthread_join");
	}

	printf("Main thread: %s\n", main_thread_dirname);

	free(threads);
	exit(EXIT_SUCCESS);
}

static void *
thread_function(void *arg) {
	struct thread_info *info = (struct thread_info *) arg;

	printf("thread %d: %s\n", info->tid, dirname_r(info->path));
	return NULL;
}

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static pthread_key_t dirname_key;

static void
dirname_data_destructor(void *buffer) {
	free(buffer);
}

static void
create_dirname_key(void) {
	int s = pthread_key_create(&dirname_key, dirname_data_destructor);
	if (s != 0)
		pthread_pexit(s, "pthread_key_create");
}

/* retrieves thread-specific data for the dirname_r function for the running
 * thread */
static void *
get_dirname_specific() {
	int s;
	s = pthread_once(&once_control, create_dirname_key);
	if (s != 0)
		pthread_pexit(s, "pthread_once");

	void *buf = pthread_getspecific(dirname_key);
	if (buf == NULL) {
		buf = malloc(PATH_MAX);
		if (!buf)
			pexit("malloc");

		s = pthread_setspecific(dirname_key, buf);
		if (s != 0)
			pthread_pexit(s, "pthread_setspecific");
	}

	return buf;
}

static char *
dirname_r(char *path) {
	char *buf = get_dirname_specific();
	int pathlen = strlen(path);

	/* if path is NULL or the empty string, dirname should return "." */
	if (!path || !strncmp(path, "", PATH_MAX)) {
		strncpy(buf, ".", PATH_MAX);
		return buf;
	}

	/* remove trailing slash, if any */
	if (path[pathlen - 1] == '/')
		path[pathlen - 1] = '\0';

	char *p = strrchr(path, '/');
	if (p) *p = '\0';
	strncpy(buf, path, PATH_MAX);
	return buf;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s <main-thread-path> <thread1-path> [...]\n", progname);
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
