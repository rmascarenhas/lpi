/* sched_fifo_demo.c - demonstrates the SCHED_FIFO scheduling policy.
 *
 * The SCHED_FIFO realtime scheduling policy uses a First In, First Out approach
 * to determine which process should use the CPU at any given time. According
 * to this strategy, the process with the highest priority should run up to
 * completion, or until it explicitly releases the CPU (via sched_yeild(2)).
 *
 * This program demonstrates this policy by placing itself in the SCHED_FIFO
 * policy and then forking. Both parent and child will run for 3 seconds, printing
 * a message after every quarter of second. After every second, control is passed
 * to the other process via sched_yield(2).
 *
 * Both processes will have its affinity set to the same core, to demonstrate
 * the policy.
 *
 * Usage
 *
 *    $ ./sched_fifo_demo
 *
 * 	This program has to have setuid-root privileges in order to
 * 	assign the desired privilege to the command. However, the privileges are
 * 	dropped before executing the command.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _GNU_SOURCE

#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void pexit(const char *fCall);
static void run_loop(const char *label);
static double wall_time(const struct tms *t);

int
main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[]) {
	struct sched_param param;
	cpu_set_t mask;
	int prio_min, prio_max;

	/* get min and max priority */
	if ((prio_min = sched_get_priority_min(SCHED_FIFO)) == -1)
		pexit("sched_get_priority_min");

	if ((prio_max = sched_get_priority_max(SCHED_FIFO)) == -1)
		pexit("sched_get_priority_max");

	param.sched_priority = (prio_min + prio_max) / 2;

	/* place itself in the SCHED_FIFO policy */
	if (sched_setscheduler(0, SCHED_FIFO, &param) == -1)
		pexit("sched_setscheduler");

	/* update the CPU affinity to make sure parent and child are run in the
	 * same processor (or core) */
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1)
		pexit("sched_setaffinity");

	/* create a child process */
	switch (fork()) {
		case -1:
			pexit("fork");
			break;
		case 0:
			run_loop("Parent");
			break;
		default:
			run_loop("Child");
			break;
	}

	/* wait for the possible children that might not have finished yet
	 * (no efect on the child process) */
	int status;
	wait(&status);

	/* finish successfully */
	exit(EXIT_SUCCESS);
}

static void
run_loop(const char *label) {
	int curr_quarter, total_quarters;
	struct tms tdata;

	/* both parent and child should loop and:
	 *
	 * - print a message every quarter of second;
	 * - yield the CPU after every second;
	 * - terminate after 3 seconds.
	 */
	curr_quarter = 0;
	total_quarters = 12; /* 3 seconds */

	for (;;) {
		if (times(&tdata) == (clock_t) -1)
			pexit("times");

		if (wall_time(&tdata) >= (curr_quarter + 1) * 0.25) {
			printf("[%s] 1/4 of second.\n", label);
			++curr_quarter;

			if (curr_quarter % 4 == 0) {
				printf("[%s] Yielding\n", label);
				if (sched_yield() == -1)
					pexit("sched_yield");
			}

			if (curr_quarter == total_quarters) {
				printf("[%s] Finished\n", label);
				break;
			}
		}
	}
}

static double
wall_time(const struct tms *t) {
	int ticks_per_sec = sysconf(_SC_CLK_TCK);

	return ((double) t->tms_utime) / ticks_per_sec;
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
