/* exec_no_setpgid.c - demonstrates that a parent cannot change a child process
 *                     group after a call to `exec` has been made.
 *
 * To avoid from having its process group inadvertently changed, SUSv3 forbids
 * a parent to change the process group of its children after they have performed
 * an `exec` operation.
 *
 * To demonstrate this behavior, the parent process will create a child that
 * will sleep for 5 seconds, and then perform an `exec` operation. The parent
 * process will wait for input from the user. At the moment it is received,
 * an attempt to change the child process group is made. Therefore, the user
 * can decide if it will be done before or after the `exec` call. The parent
 * also sets up an alarm to avoid waiting on input forever.
 *
 * Usage:
 *
 *   $ ./exec_no_setpgid
 *
 * Instructions on usage are given to the user upon execution.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void pexit(const char *fCall);

int
main() {
	pid_t child_pid;
	/* params for the `sleep` call the child is going to perform */
	char *params[] = { "sleep", "5", NULL };

	printf(">> Parent process (PID=%ld PPID=%ld PGID=%ld)\n",
			(long) getpid(), (long) getppid(), (long) getpgrp());

	printf(">> [Parent] creating child. Press Return when an attempt to change "
			"the child process group is to be made\n\n");

	switch(child_pid = fork()) {
		case -1:
			pexit("fork");
			break;

		case 0:
			/* child process. Sleep for some time to give user
			 * the chance to try an attempt to change the process
			 * group and then `exec` */
			printf("\n>> Child (PID=%ld PPID=%ld PGID=%ld) going to sleep\n",
					(long) getpid(), (long) getppid(), (long) getpgrp());
			sleep(5);

			printf(">> Child performing an exec\n");
			if (execvp("/usr/bin/sleep", params) == -1)
				pexit("execv");

			_exit(EXIT_FAILURE); /* in case the exec system call fails */

		default:
			/* parent falls through */
			break;
	}

	/* set up an alarm to avoid waiting on input forever */
	alarm(20);

	/* wait for user input */
	printf("Press Return to try to change the child process group");
	getchar();

	/* user has pressed the Return key */
	printf(">> [Parent] Attempting to change child process group\n");
	if (setpgid(child_pid, child_pid) == -1) {
		printf(">> Parent failed. Error message below\n");
		pexit("setpgid");
	} else {
		printf(">> Child process group successfully changed\n");
		printf(">> Waiting for child to terminate\n");
		wait(NULL);
	}

	exit(EXIT_SUCCESS);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
