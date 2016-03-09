/* rusage_children.c - shows that the RUSAGE_CHILDREN flag only gather info for waited children.
 *
 * The `getrusage(2)` system call can be used to retrieve system resources
 * information for a number of different resources. If a process has created children,
 * then the information returned also includes the resources consumed by children
 * so long as the parent process performed a `wait(2)` call for the children.
 *
 * This program demonstrates that resource information for a parent process
 * only includes data related to the children process after the `wait(2)` system
 * call has been performed.
 *
 * Usage:
 *
 *   $ ./rusage_children
 *
 * Author: Renato Mascarenhas Costa
 */

#include <limits.h>
#ifndef INT_MAX
#  include <linux/limits.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void pexit(const char *fCall);
static void debug(const char *label, const char *msg);
static void log_process_usage(void);

static const char *PARENT = "Parent";
static const char *CHILD  = "Child";

int
main(__attribute__((unused)) int argc, __attribute__ ((unused)) char *argv[]) {
	int j, i, status;

	debug(PARENT, "Forking child");

	switch (fork()) {
		case -1:
			pexit("fork");
			break;

		case 0:
			/* child */
			debug(CHILD, "Sleeping 1 second for parent to display system resources");
			sleep(1);

			debug(CHILD, "Performing some computation");
			/* simple user level computation - compile with no optimizations to check
			 * results when the child is completed */
			i = 0;
			for (j = 0; j < INT_MAX; ++j)
				i += (j % 2 ? 1 : -1);

			debug(CHILD, "Done");
			_exit(EXIT_SUCCESS);

			break;

		default:
			/* parent */
			/* display system resources before child uses CPU */
			debug(PARENT, "Getting system resource usage information");
			log_process_usage();

			debug(PARENT, "Waiting for child");
			if (wait(&status) == -1)
				pexit("wait");

			debug(PARENT, "Child is done, getting resources usage");
			log_process_usage();
			debug(PARENT, "Done");

			break;
	}

	exit(EXIT_SUCCESS);
}

static void
log_process_usage() {
	struct rusage ru;

	if (getrusage(RUSAGE_CHILDREN, &ru) == -1)
		pexit("getrusage");

	printf("System resource info:\n");
	printf("\t- CPU (user): %lds %ldms\n", (long) ru.ru_utime.tv_sec, (long) ru.ru_utime.tv_usec);
	printf("\t- CPU (system): %lds %ldms\n", (long) ru.ru_stime.tv_sec, (long) ru.ru_stime.tv_usec);
}

static void
debug(const char *label, const char *msg) {
	fprintf(stdout, "[%s] %s\n", label, msg);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
