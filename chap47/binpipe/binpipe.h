/* binpipe.h - An implementation of a binary semaphore protocol using named pipes.
 *
 * Binary semaphores allow the synchronization of multiple processes by incrementing/
 * decrementing a shared counter maintained by the kernel. System V's implementation
 * of semaphores works by allowing operations on semaphores to be done in sets,
 * with guaranteed atomicity.
 *
 * However, the resulting API is more complicated than needed. This implements
 * a binary semaphore protocol (i.e., semaphores can be either `set` or `clear`,
 * or, alternatively `reserved` or `released`.) using named pipes (FIFOs) as the
 * underlying synchronization mechanism.
 *
 * See implementation file binpipe.c for implementation details.
 *
 * Author: Renato Mascarenhas Costa
 */

#ifndef BINPIPE_H
#define BINPIPE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* when the semaphore is reserved, an attempt to write a byte to the pipe is made -
 * the byte defined on BP_RELEASED_BYTE. Therefore, when a process wishes to
 * release the semaphore, the underlying implementation will try to read
 * from the pipe, unblocking the process trying to reserve. */
#define BP_RELEASED_BYTE ('\0')

/* template used in calls to `mktemp(3)` in order to generate temp files
 * for use as named pipes */
#define BP_FIFO_TEMPLATE ("/tmp/bpf.XXXXXX")

extern bool bpRetryOnEintr; /* retry if a blocked system call is interrupted by a signal */

struct bpsem_t {
	char *path;  /* path to the named FIFO */
	int readFd;  /* read end of the pipe */
	int writeFd; /* write end of the pipe */
};

/* builds a `bpsem_t` data structure for a binary semaphore on top of a FIFO
 * on the `path` given. The file on that path must exist and be a valid, previously
 * created FIFO.
 *
 * Returns a pointer to a `bpsem_t` struct on success, or NULL on error.
 */
struct bpsem_t *bpInit(char *path);

/* creates a new binary semaphore. A new semaphore is never reserved - reservations
 * need to be made afterwards via `bpReserve`
 *
 * Returns an implementation defined representation of the created semaphore, which
 * must be passed to the other functions of this library, or NULL on failure.
 */
struct bpsem_t *bpCreate();

/* reserves a semaphore. If it is already reserved, this call will block. Whether
 * the operation will retry if the process receives a signal is controlled by
 * the bpRetryOnEintr variable.
 *
 * Returns 0 on success, or -1 on error (with `errno` properly set.)
 */
int bpReserve(struct bpsem_t *sem);

/* releases a semaphore on the `path` given
 *
 * Returns 0 on success, or -1 on error (with `errno` properly set.)
 */
int bpRelease(struct bpsem_t *sem);

/* conditionally reserves a (previously created) semaphore. If it is already
 * reserved, the function returns immediately.
 *
 * Returns 0 on success, or -1 on error (with `errno` properly set.) When the
 * semaphore is already reserved, `errno` is set to `EAGAIN`.
 */
int bpCondReserve(struct bpsem_t *sem);

/* destroys a previously created semaphore.
 *
 * Returns 0 on success, or -1 on error (with `errno` properly set.)
 */
void bpDestroy(struct bpsem_t *sem);

#endif /* BINPIPE_H */
