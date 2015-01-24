/* free_and_sbrk.c - shows when a call to `malloc(3)` changes the program break.
 *
 * The `malloc(3)` family of functions use the `brk(2)` system call to change the
 * program break, i.e. allocate more memory on the heap to the calling process.
 * This program aims to demonstrate that this function does not call the underlying
 * system call on every memory allocation, but instead allocates memory on the
 * heap in chunks so as to avoid unnecessary system calls.
 *
 * This is an adaptation of Listing 7-1 of "The Linux Programming Interface" book.
 *
 * Usage:
 *
 * 	$ ./free_and_sbrk <num-allocs> <block-size> [step [min [max]]]
 * 	num-allocs - the number of allocations to perform
 * 	block-size - the size of each memory block
 * 	step       - step by which to increment when freeing memory blocks (default: 1)
 * 	min        - the index of the first memory block freeing will start (default: 1)
 * 	max        - the index of the last memory block that will be freed (default: num-allocs)
 *
 * Changes by: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE
#define MAX_ALLOCS (1000000)

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#ifndef LONG_MIN
#  include <linux/limits.h>
#endif

static char *progname = "free_and_sbrk";

static void usage();
static void error(const char *msg);
static void pexit(const char *func);
static long getInt(const char *arg);

int
main(int argc, char *argv[]) {
	char *ptr[MAX_ALLOCS];
	long freeStep, freeMin, freeMax, blockSize, numAllocs, j;
	void *breakp, /* current position of the program break */
	     *breakp_iter;

	if (argc < 3)
		usage();

	numAllocs = getInt(argv[1]);
	blockSize = getInt(argv[2]);

	freeStep = (argc > 3) ? getInt(argv[3]) : 1;
	freeMin  = (argc > 4) ? getInt(argv[4]) : 1;
	freeMax  = (argc > 5) ? getInt(argv[5]) : numAllocs;

	if (freeMax > numAllocs)
		error("freeMax > numAllocs");

	breakp = sbrk(0);
	printf("%50s%10p\n", "Initial program break:", breakp);
	printf("Allocating %ld * %ld bytes\n", numAllocs, blockSize);

	for (j = 0; j < numAllocs; ++j) {
		ptr[j] = malloc(blockSize);
		if (ptr[j] == NULL)
			pexit("malloc");

		/* indicate when program break was changed */
		breakp_iter = sbrk(0);
		if (breakp_iter != breakp) {
			printf("(%ld)%50s%10p\n", j + 1, "=> Program break now at", breakp_iter);
			breakp = breakp_iter;
		}
	}

	printf("%50s%10p\n", "Program break is now:", sbrk(0));

	printf("Freeing blocks from %ld to %ld in steps of %ld\n", freeMin, freeMax, freeStep);

	for (j = freeMin - 1; j < freeMax; j += freeStep)
		free(ptr[j]);

	printf("%50s%10p\n", "After free(), program break is:", sbrk(0));

	exit(EXIT_SUCCESS);
}

static long
getInt(const char *arg) {
	long narg;
	char *endptr;

	narg = strtol(arg, &endptr, 10);
	if (endptr == arg || narg == LONG_MIN || narg == LONG_MAX) {
		error("%s is not an int");
	}

	return narg;
}

static void
error(const char *msg) {
	fprintf(stderr, "%s: %s\n", progname, msg);
	exit(EXIT_FAILURE);
}

static void
pexit(const char *func) {
	perror(func);
	exit(EXIT_FAILURE);
}

static void
usage() {
	fprintf(stderr, "Usage: %s <num-allocs> <block-size> [step [min [max]]]\n", progname);
	exit(EXIT_FAILURE);
}
