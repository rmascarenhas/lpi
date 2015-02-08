/* orphan_pg_eio.c - demonstrates a process in an orphaned process group receives
 *                   an error when it tries to perform a read(2) call.
 *
 * If a read(2) operation is attempted by a process that does not belong to
 * the current foreground process group, the kernel automatically sends it a
 * SIGTTIN signal, whose default action is to suspend the process. However, if
 * the process belongs to an orphaned process group, there is no reason for the
 * signal to be sent, otherwise the process would be suspended and no longer be
 * able to continue.
 *
 * This program shows that, if that is the case, a call to `read(2)` will instead
 * return the EIO error instead of sending the signal.
 *
 * Usage:
 *
 *   $ ./orphan_pg_eio
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void pexit(const char *fCall);

int
main() {
	/* make stdout unbuffered */
	setbuf(stdout, NULL);

	/* create an orphaned process group - fork and make the child create its
	 * own process group, then terminate the parent. */

	printf("[Parent] creating a child\n");
	switch(fork()) {
		case -1:
			pexit("fork");
			break;

		case 0:
			printf("[Child] created\n");
			if (setpgid(0, 0) == -1)
				pexit("setpgid");

			printf("[Child] Created its own process group\n");
			printf("[Child] Waiting for parent to complete\n");

			sleep(3);

			char buf[8];
			if (read(STDIN_FILENO, buf, 8) == -1) {
				printf("[Child] read(2) failed. Check error below.\n");
				pexit("read");
			}

			break;

		default:
			printf("[Parent] finishing in order to make orphaned process group\n");
			exit(EXIT_SUCCESS);

			break;
	}

	/* should never get here */
	exit(EXIT_FAILURE);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
