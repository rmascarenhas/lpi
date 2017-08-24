/* nlm.c - Implements non-linear mapping using the MAP_FIXED option to mmap(2).
 *
 * Non-linear mapping describes the situation in which the order of the pages in
 * a memory mapping is not the same as the order of the pages in the underlying
 * file. Such a situation can be created by using the MAP_FIXED option to the
 * mmap(2) system call and loading different offsets of a file.
 *
 * Alternatively, Linux provides the non-standard remap_file_pages(2) system
 * call to optimize the operation. However, this system call is currently
 * deprecated as well, according to the manual pages.
 *
 * This program creates a non-linear mapping using the MAP_FIXED technique
 * by creating a 3-pages long memory mapping on top of the file given as
 * command-line argument, and then prints the first 3 pages of memory on
 * the file and on the block of memory, demonstrating how they differ
 * (in the same way as depicted in Figure 49-5 of the Linux Programming
 * Interface book.)
 *
 * Usage
 *
 *    $ ./nlm [file]
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>

/* number of bytes on each page to print. Should be sufficient to differentiate
 * one page from each other */
#define NLM_PEEK_BYTES (10)

static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
	if (argc != 2)
		helpAndExit(argv[0], EXIT_FAILURE);

	int fd, i, j;
	long pagesize;
	char *mem, *p, buf[NLM_PEEK_BYTES + 1]; /* peek bytes + nul character */

	fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		pexit("open");

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1)
		pexit("sysconf");

	/* create an anonymous memory mapping, 3 pages long */
	mem = mmap(NULL, 3*pagesize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (mem == MAP_FAILED)
		pexit("mmap");

	/* first memory mapping: maps the 3rd page of the underlying file */
	if (mmap(mem, pagesize, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 2*pagesize) == MAP_FAILED)
		pexit("mmap");

	/* second memory mapping: map 2nd page of the underlying file */
	if (mmap(mem + pagesize, pagesize, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, pagesize) == MAP_FAILED)
		pexit("mmap");

	/* third memory mapping: map 1st page of the underlying file */
	if (mmap(mem + (2*pagesize), pagesize, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED)
		pexit("mmap");

	/* Read first 3 pages on the mapped memory */
	printf("On memory mapping:\n");
	for (i = 0; i < 3; i++) {
		p = mem + (i*pagesize);
		printf("Page %d: ", i+1);

		for (j = 0; j < NLM_PEEK_BYTES; j++)
			printf("%c", p[j]);

		printf("\n");
	}

	/* Read first 3 pages on the underlying file */
	printf("\nOn file:\n");
	for (i = 0; i < 3; i++) {
		if (lseek(fd, i*pagesize, SEEK_SET) == (off_t) -1)
			pexit("lseek");

		if (read(fd, buf, NLM_PEEK_BYTES) == -1)
			pexit("read");
		buf[NLM_PEEK_BYTES] = '\0';

		printf("Page %d: %s\n", i+1, buf);
	}

	exit(EXIT_SUCCESS);
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
