/* sched_set_cap.c - modify a given process scheduling policy and priority
 *                   using the capalibilities model.
 *
 * Scheduling techniques are covered on chapter 35. Refer to that chapter for
 * more information on scheduling policies and switching.
 *
 * What interests this program is the fact the changing the scheduling process
 * of a given process to a real time policy requires super user privileges.
 * More especifically, the calling process must have the `CAP_SYS_NICE`
 * capability.
 *
 * This is a capability-aware program that changes the scheduling policy and
 * priority of a given process (identified by its pid) without requiring
 * super user privileges or being set-user-ID-root. However, the `CAP_SYS_NICE`
 * capability must be in the permitted capabilities set of this program.
 *
 * Usage:
 *
 * 	 # Setting the capabilities correctly
 * 	 $ sudo setcap "cap_sys_nice=p" ./sched_set_cap
 * 	 root's password:
 * 	 $ ./sched_set_cap f 10 <pid>
 *
 * 	 Options are:
 * 	 	* policy:   r (round-robin), f (FIFO), b (BATCH), i (IDLE), o (OTHER).
 * 	 	            Subject to support on the running platform.
 * 	 	* priority: the numerical value of the process' priority.
 * 	 	* pid:      the identifier of the process to be affected.
 *
 * Source code is heavily based on listing 35-2 of the Linux Programming Interface
 * book, with modifications to support capabilities.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sched.h>
#include <sys/capability.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fname);
static long readLong(const char *str);

static int requireCapability(int capability);
static int dropAllCapabilities(void);

int
main(int argc, char *argv[]) {
	int pol;
	struct sched_param sp;
	long prio, pid;

	if (argc < 3 || strchr("rfobi", argv[1][0]) == NULL)
		helpAndLeave(argv[0], EXIT_FAILURE);

	pol = (argv[1][0] == 'r') ? SCHED_RR :
		(argv[1][0] == 'f') ? SCHED_FIFO :
#ifdef SCHED_BATCH
		(argv[1][0] == 'b') ? SCHED_BATCH :
#endif
#ifdef SCHED_IDLE
		(argv[1][0] == 'i') ? SCHED_IDLE :
#endif
		SCHED_OTHER;

	prio = readLong(argv[2]);
	pid  = readLong(argv[3]);

	sp.sched_priority = prio;

	if (requireCapability(CAP_SYS_NICE) == -1)
		pexit("requireCapability");

	if (sched_setscheduler(pid, pol, &sp) == -1)
		pexit("sched_setscheduler");

	if (dropAllCapabilities() == -1)
		pexit("dropAllCapabilities");

	printf("Successfully updated process %ld.\n", pid);
	exit(EXIT_SUCCESS);
}

static int
requireCapability(int capability) {
	cap_t caps;
	cap_value_t capList[1];

	/* retrieve current capabilities */
	caps = cap_get_proc();
	if (caps == NULL)
		return -1;

	/* change settingn of given `capability` in the effective set of `caps` */
	capList[0] = capability;
	if (cap_set_flag(caps, CAP_EFFECTIVE, 1, capList, CAP_SET) == -1)
		return -1;

	/* push new set of capabilities to the kernel */
	if (cap_set_proc(caps) == -1)
		return -1;

	if (cap_free(caps) == -1)
		return -1;

	return 0;
}

static int
dropAllCapabilities() {
	cap_t empty;
	int s;

	empty = cap_init();
	if (empty == NULL)
		return -1;

	s = cap_set_proc(empty);

	if (cap_free(empty) == -1)
		return -1;

	return s;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s <policy> <priority> <pid>\n", progname);
	fprintf(stream, "policy is:\n\t'r' (Round-Robin)\n\t'f' (FIFO)"
#ifdef SCHED_BATCH
			"\n\t'b' (BATCH)"
#endif
#ifdef SCHED_IDLE
			"\n\t'i' (IDLE)"
#endif
			"\n\t'o' (OTHER)");

	exit(status);
}

static void
pexit(const char *fname) {
	perror(fname);
	exit(EXIT_FAILURE);
}

static long
readLong(const char *str) {
	char *endptr;
	long l;

	l = strtol(str, &endptr, 10);
	if (endptr == str || l == LONG_MIN || l == LONG_MAX)
		pexit("strtol");

	return l;
}
