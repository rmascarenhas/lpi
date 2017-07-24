/* binpipe.c - An implementation of a binary semaphore protocol using named pipes.
 *
 * This implements the library functions for the binary protocol implementation.
 * See the `binpipe.h` file for more information.
 *
 * Author: Renato Mascarenhas Costa
 */

#include "binpipe.h"

/* WARNING: this implementation relies on named pipes (FIFOs) and its blocking
 * behaviour when reading from a pipe without any data. However, note that a double
 * `bpRelease` call will lead to likely unexpected behaviour: namely, two `bpReserve`
 * calls in a row will succeed in this case. Therefore, users of this library must
 * ensure that semaphores are released correctly, only once after use. */

struct bpsem_t *
bpInit(char *path) {
	struct bpsem_t *sem = malloc(sizeof(struct bpsem_t));
	if (sem == NULL) {
		return NULL;
	}

	int rfd, wfd, flags;

	/* since there might be no other process with a write file descriptor
	 * for this FIFO, the `open(2)` call needs to be made non-blocking, so that
	 * a read file descriptor will be created even in that situation */
	rfd = open(path, O_RDONLY | O_NONBLOCK);
	if (rfd == -1) {
		return NULL;
	}

	wfd = open(path, O_WRONLY);
	if (wfd == -1) {
		return NULL;
	}

	/* revert the read file descriptor back to being blocking, so that reserve
	 * operations on the semaphore will properly wait */
	flags = O_RDONLY;
	if (fcntl(rfd, F_SETFL, flags) == -1) {
		return NULL;
	}

	sem->path = path;
	sem->readFd = rfd;
	sem->writeFd = wfd;

	return sem;
}

struct bpsem_t *
bpCreate() {
	/* uses `mktemp(3)` to create a temporary file name, and uses that as the path
	 * to a named FIFO to be used as the underlying synchronization mechanism */
	char *fname;
	struct bpsem_t *sem;

	fname = malloc(PATH_MAX);
	if (fname == NULL) {
		return NULL;
	}

	strcpy(fname, BP_FIFO_TEMPLATE);
	mktemp(fname);

	if (mkfifo(fname, 0600) == -1) {
		return NULL;
	}

	sem = bpInit(fname);

	/* semaphores are created released - to reserve the associated resource, an
	 * explicit call to `bpReserve` must be made. */
	if (bpRelease(sem) == -1) {
		return NULL;
	}

	return sem;
}

int
bpReserve(struct bpsem_t *sem) {
	char buf[1];
	ssize_t numRead;

	/* if the pipe is empty, `read(2)` will block until another process releases
	 * this semaphore (i.e., something is written to the pipe.) */
	while (true) {
		numRead = read(sem->readFd, buf, 1);

		/* if interrupted by a signal, behaviour depends on the definition of the
		 * `bpRetryOnEintr` variable, defined by the program using this library */
		if (numRead == -1 && errno == EINTR && bpRetryOnEintr) {
			continue;
		}

		if (numRead < 1) {
			return -1;
		}

		/* successfully read from the pipe */
		break;
	}

	return 0;
}

int
bpRelease(struct bpsem_t *sem) {
	char buf[1];

	buf[0] = BP_RELEASED_BYTE;

	if (write(sem->writeFd, buf, 1) < 1) {
		return -1;
	}

	return 0;
}

int
bpCondReserve(struct bpsem_t *sem) {
	/* non-blocking read on the read end of the pipe - a successful read means
	 * the semaphore was successfully reserved; an error means the call would block,
	 * meaning the semaphore is already reserved by another process */
	int nbfd;
	char buf[1];

	nbfd = open(sem->path, O_RDONLY | O_NONBLOCK);
	if (nbfd == -1) {
		return -1;
	}

	if (read(nbfd, buf, 1) == -1) {
		/* if errno is EAGAIN, that will be propagated to the caller, which must
		 * interpret that to mean that the conditional reserve failed. If the errno
		 * is anything else, than it must also be handled by the caller. */
		return -1;
	}

	close(nbfd);
	return 0;
}

void
bpDestroy(struct bpsem_t *sem) {
	unlink(sem->path);
	close(sem->readFd);
	close(sem->writeFd);
	free(sem);
}
