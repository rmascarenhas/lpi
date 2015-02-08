/* ignored_ttin.c - shows that a SIGTTIN/SITTOU signal is not delivered if the process
 *                  is within an orphaned process group without a signal handler.
 *
 * If a process within an orphaned process group is sent a SIGTTIN or a SIGTTOU,
 * whose default action is to suspend the receiving process, then the signal is
 * just ignored even the the disposition is SIG_DFL. This is done in order to
 * avoid the situation of maintaining unreachable suspended processes that would
 * remain that way forever.
 *
 * However, if a signal disposition was set to a custom function, then the signal
 * is properly sent, since the default action of suspending the process will not
 * be taken (unless explicitly by the developer).
 *
 * This program demonstrates this behavior. If it is given a command line argument,
 * then a signal handler for SIGTTIN and SIGTTOUT is installed. It will then wait
 * for a signal (you can use kill(1) to send it signals).
 *
 * Usage:
 *
 *   $ ./ignored_ttin [arg]
 *
 *   arg - if given, a signal handler for SIGTTIN/SIGTTOU is installed.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pexit(const char *fCall);

static void handler(int signal);

int
main(int argc, __attribute__((unused)) char *argv[]) {
	/* make stdout unbuffered */
	setbuf(stdout, NULL);

	/* an argument was given - install signal handlers */
	if (argc > 1) {
		struct sigaction sa;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = handler;

		if (sigaction(SIGTTIN, &sa, NULL) == -1)
			pexit("sigaction");

		if (sigaction(SIGTTOU, &sa, NULL) == -1)
			pexit("sigaction");

		printf("[Parent] Handlers for SIGTTIN and SIGTTOU installed.\n");
	}

	printf("[Parent] Creating child and making it part of an orphaned process group\n");

	switch(fork()) {
		case -1:
			pexit("fork");
			break;

		case 0:
			printf("[Child] Waiting for parent to terminate\n");
			sleep(3);

			printf("[Child] Waiting for signals (PID=%ld)\n", (long) getpid());
			pause();

			printf("[Child] Point made. Terminating\n");
			exit(EXIT_SUCCESS);

		default:
			printf("[Parent] Terminating\n");
			_exit(EXIT_SUCCESS);
	}

	/* should never get here */
	exit(EXIT_FAILURE);
}

static void
handler(int signal) {
	printf("Caught signal %d: %s\n", signal, strsignal(signal));
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
