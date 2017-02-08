/* ftok.c - An implementation of the ftok(3) library function.
 *
 * System V IPC mechanisms (message queues, semaphores and shared memory) employ
 * a system-wide `key` (unless `IPC_PRIVATE` is used) to identify IPC data structures.
 * This means that the generation of the key must be such as to minimize the chances
 * of collisions across other IPC data structures created on the same system.
 *
 * This program illustrates the general algorithm employed by the `ftok(3)` library
 * function. Note that the algorithm itself is implementation defined and can vary
 * across operating systems. This code satisfies the requirements made by SUSv3.
 *
 * 		key_t ftok(char *pathname, int proj)
 *
 * 	pathname - the full path to an existing file
 * 	proj - projection.
 *
 * 	Requirements:
 * 	- only 8 least significant bits of `int` are used
 * 	- the function fails if the pathname given refers to a non-existent file
 * 	- Different pathnames pointing to the same file must yield the same key when
 * 	  generated with the same projection.
 *
 * The algorithm implemented in this program uses the same algorithm employed by
 * `ftok(3)` on Linux: the returned key is composed of:
 *
 * 1. Least significant 8 bits from `proj`
 * 2. Least significant 8 bits from the device number of the file at `path`
 * 3. Least significant 16 bits from the i-node number of the file at `path`
 *
 * Usage
 *
 *    $ ./ftok [file] [proj]
 *
 * Based on the description of this functionality included in "The Linux Programming
 * Interface" book.
 *
 * Compile with -DFTOK_DEBUG=1 to get debug output on the numbers used to compose
 * the final IPC key.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>

static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

static key_t _ftok(char *pathname, int proj);

int
main(int argc, char *argv[]) {
	if (argc != 3)
		helpAndExit(argv[0], EXIT_FAILURE);

	long proj;
	char *endptr;

	/* if the `proj` given is not a valid number, not greater than zero, or overflows,
	 * print the usage message and abort execution */
	proj = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0' || proj == LONG_MIN || proj == LONG_MAX || proj <= 0)
		helpAndExit(argv[0], EXIT_FAILURE);

	key_t generated_key, system_key;

	generated_key = _ftok(argv[1], (int) proj);
	system_key = ftok(argv[1], (int) proj);

	if (generated_key == -1)
		pexit("_ftok");

	if (system_key == -1)
		pexit("ftok");

	printf("_ftok: \t0x%x\n", generated_key);
	printf("ftok:  \t0x%x\n", system_key);

	exit(EXIT_SUCCESS);
}

static key_t
_ftok(char *pathname, int proj) {
	struct stat info;

	if (stat(pathname, &info) == -1)
		return -1;

	int projbits = 0,
		devbits = 0,
		inodebits = 0;
	key_t key = 0;

	projbits = proj & 0xff;
	devbits = info.st_dev & 0xff;
	inodebits = info.st_ino & 0xffff;

#if FTOK_DEBUG
	fprintf(stderr, "Projection: 0x%x\n", proj);
	fprintf(stderr, "Device Number: 0x%x\n", (unsigned int) info.st_dev);
	fprintf(stderr, "i-node number: 0x%x\n\n", (unsigned int) info.st_ino);

	fprintf(stderr, "Projection bits: 0x%x\n", projbits);
	fprintf(stderr, "Device Number bits: 0x%x\n", devbits);
	fprintf(stderr, "i-node number bits: 0x%x\n", inodebits);
	fprintf(stderr, "\n");
#endif

	key |= projbits;
	key <<= 8;

	key |= devbits;
	key <<= 16;

	key |= inodebits;

	return key;
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [file] [proj]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
