/* shmcreate.c - Creates a System V shared memory segment.
 *
 * Utility program designed to create a shared memory segment with any size.
 * Useful in order to test the behaviour of the other programs in this directory.
 * If successful, this program prints the numerical identifier associated with
 * the new shared memory segment.
 *
 * Usage
 *
 *    $ ./shmcreate [size]
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>

#define SHM_PERMS (S_IRUSR | S_IWUSR)

static void fatal(const char *msg);
static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
	int shmid;
	long size;
	char *endptr;

	if (argc != 2)
		helpAndExit(argv[0], EXIT_FAILURE);

	size = strtol(argv[1], &endptr, 10);
	if (size <= 0 || size == LONG_MAX || *endptr != '\0')
		fatal("Invalid size argument.");

	shmid = shmget(IPC_PRIVATE, (size_t) size, IPC_CREAT | IPC_EXCL | SHM_PERMS);
	if (shmid == -1)
		pexit("shmget");

	printf("%d\n", shmid);
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

	fprintf(stream, "Usage: %s [size]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
