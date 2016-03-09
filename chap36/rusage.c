/* rusage.c - show resource usage for a given command.
 *
 * This program provides a functionality that is similar to the
 * time(1) utility, but instead of only providing timing information,
 * it also produces other kinds of resource usage by the command
 * passed.
 *
 * Usage:
 *
 *   $ ./rusage command arg ...
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void exec_start_handler(int signal);

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static volatile sig_atomic_t exec_start = false;

int
main(int argc, char *argv[]) {
	struct sigaction sa;
	pid_t child_pid;

	if (argc < 2)
		helpAndLeave(argv[0], EXIT_FAILURE);

	/* wait for parent indicate that it is waiting for the child */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = exec_start_handler;

	if (sigaction(SIGCONT, &sa, NULL) == -1)
		pexit("sigaction");

	/* fork a child and execute the command */

	switch (child_pid = fork()) {
		case -1:
			pexit("fork");
			break;

		case 0:
			/* child: execute the command */
			while (!exec_start)
				pause();

			/* parent is ready, execute the command */
			if (execvp(argv[1], &argv[1]) == -1)
				pexit("exec");

			break;

		default:
			/* parent: wait for command to finish and then print
			 * resource usage information */

			/* tell child that we are ready to wait. This still is not guaranteed
			 * to work, but is a safer approach to just running the child as soon
			 * as it is ready. */
			kill(child_pid, SIGCONT);
			int status;

			if (wait(&status) == -1)
				pexit("wait");

			/* print resource usage */
			struct rusage ru;

			if (getrusage(RUSAGE_CHILDREN, &ru) == -1)
				pexit("getrusage");

			printf("\n");
			printf("CPU (user): %lds %ldms\n", (long) ru.ru_utime.tv_sec, ru.ru_utime.tv_usec);
			printf("CPU (system): %lds %ldms\n", (long) ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);
			printf("Page reclaims: %ld\n", ru.ru_minflt);
			printf("Page faults: %ld\n", ru.ru_majflt);
	}

	exit(EXIT_SUCCESS);
}

static void
exec_start_handler(__attribute__ ((unused)) int signal) {
	exec_start = true;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stdout;

	if (status == EXIT_FAILURE)
		stream = stderr;

	fprintf(stream, "Usage: %s [command] [args]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
