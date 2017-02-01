/* popen.c - an implementation of the popen(3) and pclose(3) functions.
 *
 * The `popen(3)` library function is able to run commands in the host shell,
 * and allow the calling process to eithe read from the child process' standard
 * output, or write to its standard input. Each call to the function invokes `/bin/sh`
 * in order to parse the command given.
 *
 * Pipes and processes created with `popen(3)` must be closed with `pclose(3)`. This
 * ensures that the file descriptors are closed, and waits for the child process.
 *
 * This program provides a simple implementation of both of these functions, as well
 * as allows the user to test its functionality
 *
 * Usage:
 *
 *   $ ./popen [type] [command]
 *   type: whether we are interested in reading (r) or writing (w).
 *   command: the command to be run
 *
 * Examples:
 *
 * 	$ ./popen r ls
 * 	upcase_pipe.c popen.c
 *
 * 	$ ./popen w cat
 * 	(waits for user input) Hello World
 * 	(echo's output) Hello World
 *
 * Author: Renato Mascarenhas Costa
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN (1024)

#define SHELL ("/bin/sh")

#define READ ('r')
#define WRITE ('w')

/* allow up to MAXFD for file descriptor identifiers */
#define MAXFD (1024)
/* maps file descriptors to child PID, allowing multiple calls to _popen */
static int fdPidMap[MAXFD];

static void init(void);
static FILE *_popen(const char *command, const char *type);
static int _pclose(FILE *f);

static void readAndPrint(FILE *f);
static void writeStdinTo(FILE *f);

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, const char *argv[]) {
	const char *command = argv[2],
		  *type = argv[1];

	/* this program accepts exactly two arguments (type and command to be run.)
	 * The type is a single character string (either r or w)
	 */
	if (argc != 3 || strlen(type) != 1)
		helpAndLeave(argv[0], EXIT_FAILURE);

	init();

	FILE *f = _popen(command, type);
	if (f == NULL)
		pexit("_popen");

	switch (type[0]) {
	case 'r':
		readAndPrint(f);
		break;

	case 'w':
		writeStdinTo(f);
		break;

	default:
		helpAndLeave(argv[0], EXIT_FAILURE);
	}

	if (_pclose(f) == -1)
		pexit("_pclose");

	exit(EXIT_SUCCESS);

}

/* initialize all elements of the fdPidMap data structure to zero, so as to properly
 * identify when a given field has not been filled yet. Since, for simplicity sake,
 * all the code is in a single source file, this function needs to be called
 * explicitly, from the main function.
 *
 * In practice, an implementation of popen/pclose could be distributed as a
 * shared library, and this function could be loaded when the library is loaded
 * by the C environment.
 */
static void
init() {
	int i;

	for (i = 0; i < MAXFD; ++i) {
		fdPidMap[i] = 0;
	}
}

static FILE *
_popen(const char *command, const char *type) {
	long childPid; /* the PID of the child shell to be forked */
	int pfd[2];    /* read/write file descriptors of the pipe */
	int closeFd, keepFd;

	/* if the type specified is not 1-character long or is not equal to r or w,
	 * it is invald - set errno appropriately */
	if (strlen(type) != 1 || !(type[0] == READ || type[0] == WRITE)) {
		errno = EINVAL;
		return NULL;
	}

	if (pipe(pfd) == -1)
		return NULL;

	switch (childPid = fork()) {
	case -1:
		return NULL;

	case 0:
		break;

	default:
		/* parent process:
		 *
		 * - closes the unused end of the pipe: the read side when writing, or the write
		 *   side when reading.
		 * - returns the other, open side of the pipe to the caller
		 *
		 * The parent always returns in this block of the switch statement.
		 */
		closeFd = (type[0] == WRITE) ? pfd[0] : pfd[1];
		keepFd = (type[0] == WRITE) ? pfd[1] : pfd[0];

		if (close(closeFd) == -1)
			return NULL;

		/* keep the FD => PID association stored in a shared data structure to be used
		 * subsequently by _pclose */
		if (keepFd >= MAXFD) {
			errno = ENOMEM;
			return NULL;
		}
		fdPidMap[keepFd] = childPid;

		return fdopen(keepFd, type);
	}

	/* child process */
	switch (type[0]) {
	case READ:
		/* if the caller wants to read data, then the read end of the pipe must be closed,
		 * and its write end associated with the child process standard output */
		if (close(pfd[0]) == -1)
			return NULL;

		if (pfd[1] != STDOUT_FILENO) {
			if (dup2(pfd[1], STDOUT_FILENO) == -1)
				return NULL;

			if (close(pfd[1]) == -1)
				return NULL;
		}

		break;

	case WRITE:
		/* if the caller wants to write data, then the write end of the pipe must be closed,
		 * and its read end associated with the child process standard input */
		if (close(pfd[1]) == -1)
			return NULL;

		if (pfd[0] != STDIN_FILENO) {
			if (dup2(pfd[0], STDIN_FILENO) == -1)
				return NULL;

			if (close(pfd[0]) == -1)
				return NULL;
		}
	}

	execlp(SHELL, SHELL, "-c", command, (char *) 0);

	/* if we get to this point, the exec call failed, and errno was set. Return NULL */
	return NULL;
}

static int
_pclose(FILE *f) {
	int fd; /* the file descriptor associated with the FILE* pointer given */
	long childPid; /* the PID of the child shell process */

	if ((fd = fileno(f)) == -1)
		return -1;

	/* invalid request - no PID can be found for the file descriptor given */
	if ((childPid = fdPidMap[fd]) == 0) {
		errno = EINVAL;
		return -1;
	}

	/* waits for the child status */
	int stat_loc;
	if (waitpid(childPid, &stat_loc, 0) == -1)
		return -1;

	return WEXITSTATUS(stat_loc);
}

static void
readAndPrint(FILE *f) {
	char buf[BUFLEN];
	ssize_t numRead;

	int fd = fileno(f);
	if (fd == -1)
		pexit("fileno");

	while ((numRead = read(fd, buf, BUFLEN)) != 0) {
		if (write(STDOUT_FILENO, buf, numRead) != numRead)
			pexit("stdout: partial write");
	}

	if (numRead == -1)
		pexit("read");
}

static void
writeStdinTo(FILE *f) {
	char buf[BUFLEN];

	int fd = fileno(f);
	if (fd == -1)
		pexit("fileno");

	/* read STDIN to get what to write to the child process from the user */
	while (fgets(buf, BUFLEN, stdin) != NULL) {
		/* remove the trailing newline character if present */
		if (buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';

		if (write(fd, buf, strlen(buf)) == -1)
			pexit("pipe: write");
	}
}


static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [type: r|w] [command]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
