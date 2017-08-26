/* mmsignals.c - Demonstrates how memory mappings can trigger SIGBUS and SIGSEGV.
 *
 * When a file memory mapping is created with a size larger than that of the
 * underlying file, it creates a unique situation: trying to access a memory
 * position in the pages for which there is no corresponding content in the
 * underlying file causes the kernel to send a SIGBUS to the offending process;
 * however, accessing a memory position outside of the memory mapping causes
 * SIGSEGV to be sent instead, which happens for any memory mapping scenario.
 *
 * This program demonstrates this behaviour by creating a memory mapping on top
 * of the file given as a command-line parameter. The memory mapping will be twice
 * the size of the actual underlying file. Supposing the file is n bytes long,
 * trying to access the any byte between n and (n + page boundary) will generate
 * SIGBUS. Additionally, accesses at addresses past (n + page boundary) will result
 * in a SIGSEGV being sent to the calling process.
 *
 * Usage
 *
 *    $ ./mmsignals [file] [s|b]
 *    [file] can be any file in the file system.
 *
 *    if last parameter is 's', the SIGSEGV scenario is replicated;
 *    'b' simulates the SIGBUS scenario.
 *
 * Author: Renato Mascarenhas Costa
 */

 /* get definition of ftruncate(2) */
#define _DEFAULT_SOURCE /* for newer versions of gcc */
#define _BSD_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

static void causeSigsegv(char *mem, long pagesize, long memorysize);
static void causeSigbus(char *mem, long filesize, long pagesize);

int
main(int argc, char *argv[]) {
	/* validate parameters: there must be exactly 1 parameter, and it must be
	 * either 's' or 'b' */
	if (!(argc == 3 && strlen(argv[2]) == 1 && (argv[2][0] == 's' || argv[2][0] == 'b')))
		helpAndExit(argv[0], EXIT_FAILURE);

	long pagesize;
	int fd;
	char *mem;
	struct stat st;

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1)
		pexit("sysconf");

	/* check the size of the file given as command line argument */
	fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		pexit("open");

	if (fstat(fd, &st) == -1)
		pexit("fstat");

	/* create a memory mapping twice that size */
	mem = mmap(NULL, 2*st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED)
		pexit("mmap");

	switch (argv[2][0]) {
		case 's':
			printf("Causing SIGSEGV...\n");
			causeSigsegv(mem, pagesize, 2*st.st_size);
			break;

		case 'b':
			printf("Causing SIGBUS...\n");
			causeSigbus(mem, st.st_size, pagesize);
			break;

		default:
			printf("Unreachable\n");
			exit(EXIT_FAILURE);
	}

	if (close(fd) == -1)
		pexit("close");

	if (munmap(mem, 2*st.st_size) == -1)
		pexit("munmap");

	exit(EXIT_SUCCESS);
}

static void
causeSigsegv(char *mem, long pagesize, long memorysize) {
	/* SIGSEGV is triggered if access is attempted at a position outside the
	 * mapped region */

	int pages;
	pages = memorysize / pagesize;

	/* attempt to read a position well past the end of the memory mapping */
	char x = mem[(pages + 10)*pagesize]; /* BOOM */
	printf("x: %c\n", x);
}

static void
causeSigbus(char *mem, long filesize, long pagesize) {
	int pages, sigbusBytes;

	pages = filesize / pagesize;
	sigbusBytes = filesize % pagesize;

	/* if the underlying file size is a multiple of the system page size,
	 * the SIGBUS scenario cannot be replicated */
	if (sigbusBytes == 0) {
		fprintf(stderr, "File size (%ldb) is a multiple of the page size (%ldb)\n", filesize, pagesize);
		exit(EXIT_FAILURE);
	}

	char x = mem[(pages + 1)*pagesize + 1]; /* BOOM */
	printf("x: %c\n", x);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [file] [s|b]\n", progname);
	fprintf(stream, "\ts - causes SIGSEGV\n");
	fprintf(stream, "\tb - causes SIGBUS\n");

	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
