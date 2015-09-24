/* rtsched.c - realtime scheduling analog of `nice(1)`.
 *
 * The nice(1) command runs a given shell utility with an adjusted niceness
 * value. This program provides the same functionality, but using a realtime
 * scheduler instead.
 *
 * Usage
 *
 *    $ ./rtsched [POLICY] [PRIORITY] [COMMAND]
 *
 * 	  - POLICY - the realtime policy to be used. Either r for SCHED_RR
 * 	  or f for SCHED_FIFO.
 * 	  - PRIORITY - the numerical value to set the priority to.
 * 	  - COMMAND - the command to be run
 *
 * 	This program has to have setuid-root privileges in order to
 * 	assign the desired privilege to the command. However, the privileges are
 * 	dropped before executing the command.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <limits.h>
#ifndef LONG_MIN
#  include <linux/limits.h>
#endif

#include <sys/types.h>
#include <sched.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);
static void fatal(const char *msg);

int
main(int argc, char *argv[]) {
	if (argc < 4)
		helpAndLeave(argv[0], EXIT_FAILURE);

	/* check that we have the permissions for running this program appropriately */
	if (geteuid() != 0)
		fatal("This program must be run as root (or setuid-root)\n");

	int policy;
	long priority;
	char *endptr;
	struct sched_param schedp;
	int priority_min, priority_max;

	/* get policy and priority from command-line arguments */
	if (!strncmp(argv[1], "r", 2))
		policy = SCHED_RR;
	else if (!strncmp(argv[1], "f", 2))
		policy = SCHED_FIFO;
	else
		fatal("Unknown scheduling policy");

	priority = strtol(argv[2], &endptr, 10);
	if (priority == LONG_MIN || priority == LONG_MAX || *endptr != '\0')
		fatal("Invalid priority\n");

	/* check given priority are within the boundaries accepted for the given
	 * policy on the running system */
	if ((priority_min = sched_get_priority_min(policy)) == -1)
		pexit("sched_get_priority_min");

	if ((priority_max = sched_get_priority_max(policy)) == -1)
		pexit("sched_get_priority_max");

	if (priority < priority_min || priority > priority_max)
		fatal("priority out of bounds\n");

	/* set the scheduler policy to the desired one */
	schedp.sched_priority = priority;
	if (sched_setscheduler(0, policy, &schedp) == -1)
		pexit("sched_setscheduler");

	/* scheduler set - drop privileges and run specified command */
	if (seteuid(getuid()) == -1)
		pexit("seteuid");

	execvp(argv[3], &argv[3]);

	/* if we get to this point, the exec call failed */
	pexit("exec");

	exit(EXIT_SUCCESS); /* unreachable */
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [POLICY] [PRIORITY] [COMMAND]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}

static void
fatal(const char *msg) {
	fprintf(stderr, msg);
	exit(EXIT_FAILURE);
}
