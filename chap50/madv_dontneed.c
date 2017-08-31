/* madv_dontneed.c - Demonstrates the effects of madvise(2) with MADV_DONTNEED.
 *
 * On Linux systems, the use of the madvise(2) system call with MADV_DONTNEED
 * advice causes the kernel to discard the associated memory pages - that is,
 * in the case of a private memory mapping of a file, subsequent reads will
 * page fault and the content will be read from the underlying file. In other
 * words, this call is an easy way to reset any previous modifications performed
 * during process execution.
 *
 * This programs illustrates this behaviour by creating a private memory mapping
 * on the file given as command line argument. It then modifies the first 3 pages
 * of the memory mapping, and prints them for verification. Finally, madvise(2)
 * is invoked with MADV_DONTNEED and the pages are shown again - this time, they
 * should reflect the original content as available in the underlying file.
 *
 * Note that this behaviour is Linux specific and other Unix implementations might
 * produce different outputs.
 *
 * Usage
 *
 *    $ ./madv_dontneed [file]
 *
 * Author: Renato Mascarenhas Costa
 */

/* get definition of madvise(2) */
#define _BSD_SOURCE
#define _DEFAULT_SOURCE /* newer versions of glibc */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

/* number of bytes show for each page to be printed in order to identify
 * its contents */
#ifndef MDN_PEEK_BYTES /* allow the compiler to override */
#	define MDN_PEEK_BYTES (10)
#endif

/* number of pages of content to be shown.
 *
 * IMPORTANT: for this program to properly demonstrate the behaviour described
 * at the top, the file given as the command line argument must be larger than
 * SHOW_PAGES long! */
#define SHOW_PAGES (3)

static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

static void peek(char *mem);

int
main(int argc, char *argv[]) {
	if (argc != 2)
		helpAndExit(argv[0], EXIT_FAILURE);

	long pagesize;
	int fd, i;
	char *mem;
	struct stat st;

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1)
		pexit("sysconf");

	fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		pexit("open");

	if (fstat(fd, &st) == -1)
		pexit("fstat");

	/* create writable, private memory mapping on the file */
	mem = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED)
		pexit("mmap");

	printf("File loaded:\n");
	peek(mem);

	for (i = 0; i < SHOW_PAGES; i++) {
		/* replace 3 first characters in each page with 'XXX' */
		mem[i*pagesize] = 'X';
		mem[i*pagesize + 1] = 'X';
		mem[i*pagesize + 2] = 'X';
	}

	printf("\nModified file:\n");
	peek(mem);

	if (madvise(mem, st.st_size, MADV_DONTNEED) == -1)
		pexit("madvise");

	printf("\nmadvise() with MADV_DONTNEED:\n");
	peek(mem);

	if (munlock(mem, st.st_size) == -1)
		pexit("munlock");

	exit(EXIT_SUCCESS);
}

/* displays the first SHOW_PAGES of the block of memory pointed to by the +mem+
 * argument. For each page, the first MDN_PEEK_BYTES are displayed, in order for
 * the user to identify each page */
static void
peek(char *mem) {
	int i, j;
	char buf[MDN_PEEK_BYTES + 1]; /* account for the nul character */

	long pagesize = sysconf(_SC_PAGESIZE);

	for (i = 0; i < SHOW_PAGES; i++) {
		for (j = 0; j< MDN_PEEK_BYTES; j++) {
			buf[j] = mem[i*pagesize + j];
		}

		buf[MDN_PEEK_BYTES] = '\0';
		printf("-> Page %d: %s...\n", i+1, buf);
	}
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [file]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
