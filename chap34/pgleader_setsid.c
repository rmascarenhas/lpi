/* pgleader_setsid.c - demonstrates that a process group leader cannot change its SID.
 *
 * A process group leader cannot change its session ID since the process hierarchy would
 * not hold otherwise - i.e., a process group would have process that belonged to different
 * sessions.
 *
 * This program demonstrates that it is not possible to change the session ID of a process
 * group leader. For that goal, it is necessary that this program is run by itself - i.e.,
 * not within a pipe.
 *
 * Usage:
 *
 *    $ ./pgleader_setsid
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void pexit(const char *fCall);

int
main() {
	/* ensure that the program is the process group leader */
	if (getpid() != getpgrp()) {
		fprintf(stderr, "This program can only be run as the process "
				"group leader. Exiting\n");

		exit(EXIT_FAILURE);
	}

	if (setsid() == -1)
		pexit("setsid");

	printf("setsid(2) succeeded. Your kernel has a bug\n");
	exit(EXIT_FAILURE);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
