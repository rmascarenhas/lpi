/* nice.c - an implementation of the `nice(1)` utility.
 *
 * The nice(1) command runs a given shell utility with an adjusted niceness
 * value. By default, the adjusted value is 10. On Linux, the niceness value
 * can range from -20 (maximum priority) to 19 (minimum priority).
 *
 * If no arguments are given, it will print the current niceness value and
 * exit.
 *
 * Usage
 *
 *    $ ./nice [-n VALUE] [COMMAND]
 *
 *    -n - the niceness adjustment. By default, it is equal to -10 (increased
 *    priority).
 *
 * Author: Renato Mascarenhas Costa
 */

#include <limits.h>
#ifndef LONG_MIN
#  include <linux/limits.h>
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
	long adjustment = -10;
	int command_start = 1;
	int niceness = getpriority(PRIO_PROCESS, 0);
	if (niceness == -1)
		pexit("getpriority");

	if (argc == 1) {
		/* if no command is given, just print the current niceness value */
		printf("%d\n", getpriority(PRIO_PROCESS, 0));

	} else {
		/* check for custom adjustment value */
		if (!strncmp(argv[1], "-n", BUFSIZ)) {
			char *endptr;
			adjustment = strtol(argv[2], &endptr, 10);
			if (adjustment == LONG_MIN || adjustment == LONG_MAX || *endptr != '\0')
				helpAndLeave(argv[0], EXIT_FAILURE);

			command_start = 3;
		}

		if (command_start >= argc)
			helpAndLeave(argv[0], EXIT_FAILURE);

		/* change niceness and exec(2) the comand - niceness is inherited in
		 * the execed program */
		if (setpriority(PRIO_PROCESS, 0, niceness + adjustment) == -1)
			pexit("setpriority");

		execvp(argv[command_start], &argv[command_start]);

		/* if we got to this point, there was an error execing the given command */
		pexit("exec");
	}

	return EXIT_SUCCESS;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [-n VALUE] [COMMAND]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
