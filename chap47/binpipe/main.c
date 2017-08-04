/* main.c - example program to make use of the FIFO based binary sempahore implementation.
 *
 * This program exercises the FIFO based binary semaphore implementation available on
 * the binpipe.h and binpipe.c files in this directory. It accepts command line parameters
 * that allow semaphores to be created, reserved, released, conditionally reserved, and
 * destroyed. Run with the '-h' flag for a list of options.
 *
 * Usage:
 *
 *		$ ./main -c
 *		/path/to/file # file to be used as identifier of the semaphore
 *
 *		$ ./main -r /path/to/file
 *		reserved
 *
 * Run with the `-h` flag for a list of options.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>

#include "binpipe.h"

#define BUF_SIZE (1024)

static void helpAndExit(const char *progname, int status);

static void createSemaphore(void);
static void reserveSemaphore(char *path);
static void releaseSemaphore(char *path);
static void condReserveSemaphore(char *path);
static void destroySemaphore(char *path);

/* defaults for the binary semaphore library */
bool bpRetryOnEintr = true;

int
main(int argc, char *argv[]) {
	char opt;

	if (argc == 1) {
		helpAndExit(argv[0], EXIT_SUCCESS);
	}

	while ((opt = getopt(argc, argv, "+cr:x:q:d:h")) != -1) {
		switch (opt) {
			case 'c':
				createSemaphore();
				break;
			case 'r':
				reserveSemaphore(optarg);
				break;
			case 'x':
				releaseSemaphore(optarg);
				break;
			case 'q':
				condReserveSemaphore(optarg);
				break;
			case 'd':
				destroySemaphore(optarg);
				break;
			case 'h':
				helpAndExit(argv[0], EXIT_SUCCESS);
				break;
		}
	}

	exit(EXIT_SUCCESS);
}

static void
fatal(const char *message) {
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

static char *
currTime() {
	static char buf[BUF_SIZE];
	time_t t;
	size_t s;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	if (tm == NULL)
		return NULL;

	s = strftime(buf, BUF_SIZE, "%Y-%m-%d %H:%M:%S", tm);
	return (s == 0) ? NULL : buf;
}

static struct bpsem_t *
semBuild(char *path) {
	struct bpsem_t *sem;
	sem = bpInit(path);

	if (sem == NULL) {
		perror("bpInit");
		fatal("Error creating semaphore");
	}

	return sem;
}

static void
createSemaphore() {
	struct bpsem_t *sem;
	sem = bpCreate();

	if (sem == NULL) {
		perror("bpCreate");
		fatal("Error creating semaphore");
	}

	printf("[%ld][%s] %s\n", (long) getpid(), currTime(), sem->path);
	printf("Press Ctrl-C when done");
	fflush(stdout);

	/* process that creates the semaphore is kept alive so that there will always
	 * be file descriptors for the underlying FIFOs. Otherwise, data written to the
	 * FIFO would be lost if all file descriptors were closed. */
	pause();
}

static void
reserveSemaphore(char *path) {
	struct bpsem_t *sem;
	sem = semBuild(path);
	int read;

	if ((read = bpReserve(sem)) == -1) {
		perror("bpReserve");
		fatal("Error reserving semaphore");
	}

	printf("[%ld][%s] %s: reserved\n", (long) getpid(), currTime(), sem->path);
}

static void
releaseSemaphore(char *path) {
	struct bpsem_t *sem;
	sem = semBuild(path);

	if (bpRelease(sem) == -1) {
		perror("bpRelease");
		fatal("Error releasing semaphore");
	}

	printf("[%ld][%s] %s: released\n", (long) getpid(), currTime(), sem->path);
}

static void
condReserveSemaphore(char *path) {
	struct bpsem_t *sem;
	sem = semBuild(path);

	if (bpCondReserve(sem) == -1) {
		if (errno == EAGAIN) {
			printf("[%ld][%s] %s: already reserved\n", (long) getpid(), currTime(), sem->path);
			return;
		} else {
			perror("bpCondReserve");
			fatal("Error conditionally reserving semaphore");
		}
	}

	printf("[%ld][%s] %s: reserved\n", (long) getpid(), currTime(), sem->path);
}

static void
destroySemaphore(char *path) {
	struct bpsem_t *sem;
	sem = semBuild(path);

	bpDestroy(sem);
	printf("[%ld][%s] %s: destroyed\n", (long) getpid(), currTime(), path);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [-c] [-r [path]] [-x [path]] [-q [path]] [-d [path]]\n", progname);
	fprintf(stream, "\t%-10s%-50s\n", "-c", "Creates a new semaphore");
	fprintf(stream, "\t%-10s%-50s\n", "-r", "Reserves an existing semaphore");
	fprintf(stream, "\t%-10s%-50s\n", "-x", "Releases a semaphore");
	fprintf(stream, "\t%-10s%-50s\n", "-q", "Conditionally reserves a semaphore");
	fprintf(stream, "\t%-10s%-50s\n", "-d", "Deletes a semaphore");

	exit(status);
}
