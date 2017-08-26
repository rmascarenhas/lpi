/* mmcp.c - A simple implementation of cp(1) using memory mapped I/O.
 *
 * In its simplest case, the `cp(1)` utility copies the contents of a file
 * into a new (or existing) file. Traditionally, this can be done using the
 * file I/O operations - namely, read(2) and write(2). However, this tools
 * demonstrates the usage of memory mapped I/O by using memory mappings in
 * order to do the copying, thus completely obviating the need for read(2)
 * and write(2).
 *
 * Usage
 *
 *    $ ./mmcp [src] [dst]
 *
 * Author: Renato Mascarenhas Costa
 */

 /* get definition of ftruncate(2) */
#define _DEFAULT_SOURCE /* for newer versions of gcc */
#define _BSD_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
	int srcfd, dstfd;
	struct stat st;
	void *srcmem, *dstmem;

	if (argc != 3)
		helpAndExit(argv[0], EXIT_FAILURE);

	srcfd = open(argv[1], O_RDONLY);
	if (srcfd == -1)
		pexit("open");

	/* fstat(2) the input file to get its size */
	if (fstat(srcfd, &st) == -1)
		pexit("fstat");

	/* create memory mapping on the input file */
	srcmem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
	if (srcmem == MAP_FAILED)
		pexit("mmap");

	/* open the output file and truncate it to the size of the input file */
	dstfd = open(argv[2], O_CREAT | O_RDWR, st.st_mode); /* maintain permissions */
	if (dstfd == -1)
		pexit("open");

	if (ftruncate(dstfd, st.st_size) == -1)
		pexit("ftruncate");

	/* create a memory mapping for the output file - the mapping must be shared so
	 * that changes in the block of memory are carried through the underlying file */
	dstmem = mmap(NULL, st.st_size, PROT_WRITE, MAP_SHARED, dstfd, 0);
	if (dstmem == MAP_FAILED)
		pexit("mmap");

	/* copies memory from the input file mapped memory to the output file
	 * mapped memory */
	memcpy(dstmem, srcmem, st.st_size);

	if (close(srcfd) == -1)
		pexit("close");

	if (close(dstfd) == -1)
		pexit("close");

	if (munmap(srcmem, st.st_size) == -1)
		pexit("munmap");

	/* makes sure the memory is flushed to the output file before terminating */
	if (msync(dstmem, st.st_size, MS_SYNC) == -1)
		pexit("msync");

	if (munmap(dstmem, st.st_size) == -1)
		pexit("munmap");

	exit(EXIT_SUCCESS);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [src] [dst]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
