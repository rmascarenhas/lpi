/* showshm.c - Show information about a specific shared memory segment.
 *
 * Shared memory allow processes to share the same underlying physical memory
 * by attaching a shared memory segment to their virtual address space. It allows
 * efficient IPC but requires additional logic to avoid data races between
 * processes.
 *
 * This program displays all the available information kept by the kernel about
 * a System V shared memory segment, given its numerical identifier.
 *
 * Usage
 *
 *    $ ./showshm [id]
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/shm.h>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>

static void fatal(const char *msg);
static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
	if (argc != 2)
		helpAndExit(argv[0], EXIT_FAILURE);

	struct shmid_ds data;
	char *endptr;
	long shmid;

	shmid = strtol(argv[1], &endptr, 10);
	if (shmid < 0 || shmid == LONG_MAX || *endptr != '\0')
		fatal("id must be a number");

	if (shmctl(shmid, IPC_STAT, &data) == -1)
		pexit("shmctl");

	printf("Key: 0x%08x\n", data.shm_perm.__key);
	printf("Size (bytes): %ld\n", data.shm_segsz);
	printf("Owner (UID): %ld\n", (long) data.shm_perm.uid);
	printf("PID of creator: %ld\n", (long) data.shm_cpid);
	printf("Processes attached: %ld\n", (long) data.shm_nattch);

	exit(EXIT_SUCCESS);
}

static void
fatal(const char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stdout;

	if (status == EXIT_FAILURE)
		stream = stderr;

	fprintf(stream, "Usage: %s [shared-memory-id]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
