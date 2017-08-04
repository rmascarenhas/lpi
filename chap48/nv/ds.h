/* ds.h - implementation of a simple key/value data structure.
*
* This provides a simple key/value data store, to be used as the main storage
* mechanism * of the `nv` tool. It provides functionalities similar to those of
* a hash table or dictionary. Only strings are supported as name and value.
*
* The underlying implementation uses a simple array based solution. Therefore,
* every lookup is O(n). The goal is just to simulate a hash table API - implementing
* an efficient hashing algorithm is outside of the scope of this project.
*
* Author: Renato Mascarenhas Costa
*/

#ifndef DS_H
#define DS_H

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sems.h"

/* allow the compiler to override these constants */
#ifndef NVDS_NAME_LEN
#	define NVDS_NAME_LEN (1024)
#endif

#ifndef NVDS_VAL_LEN
#	define NVDS_VAL_LEN (4098)
#endif

/* memory layout of the data structure:
 *
 * first (sizeof int) bytes: the size of the array
 * next (sizeof int) bytes: the array's capacity
 * next (sizeof int) bytes: the semaphore id used to synchronize read operations
 * next (sizeof int) bytes: the semaphore id used to synchronize write operations
 * remaining bytes: the actual array of `struct nvds_entry` elements
 *
 * This ensures that such blocks of memory can be used with shared memory segments.
 * Using `dsInit`, the first metadata bytes are set up so that they can
 * later be manipulated in subsequent calls (using the stored `base` address.)
 *
 * The semaphore IDs stored in the memory block is the identifiers of System V
 * semaphores, managed via the binary semaphore API provided by `sems.h`. The semaphores
 * ensures that reads may happen concurrently, but that no reads or writes can happen
 * while a write operation is halfway.
 */

#define DS_SIZE_OFFSET  (0)
#define DS_CAP_OFFSET   (1)
#define DS_RLOCK_OFFSET (2)
#define DS_WLOCK_OFFSET (3)

/* flags to be used on `dsLock` and `dsUnlock` functions to indicate which operations
 * should be locked */
#define DS_READ       (1 << 0)
#define DS_WRITE      (1 << 1)
#define DS_READ_WRITE (DS_READ | DS_WRITE)

struct nvds_entry {
	char name[NVDS_NAME_LEN]; /* name */
	char val[NVDS_VAL_LEN];   /* value */
};

/* receives a block of memory at the given address and initializes internal
 * data structures for the requested capacity.
 *
 * The `mem` block of memory should be created by the caller, preferrably
 * delegating the calculation of an appropriate size using the `dsCapToBytes`
 * function.
 *
 * Returns -1 on error */
int dsInit(void *mem, int cap);

/* given a capacity, this returns the number of bytes necessary for the storage
 * of the requested number of name/value pairs, plus any required metadata */
int dsCapToBytes(int cap);

/* Receives a previously created block of memory `mem` and validates whether its
 * metadata is valid (i.e., the internal data structures are consistent and the
 * lock identifiers still exist in the system.)
 *
 * Returns 0 on success or -1 on error. */
int dsValidate(void *mem);

/* creates an entry with the given `name` and value. If an entry for the name
 * provided already exists, it is updated.
 *
 * Returns -1 on error. If there are no slots available for insertion, errno
 * is set to ENOMEM. */
int dsSet(void *mem, char *name, char *val);

/* gets the value associated wit the `name` given. Copies the result
 * on the `buf` given, up to `size` bytes.
 *
 * Returns -1 on error. When the name is not found, errno is set to EINVAL. */
int dsGet(void *mem, char *name, char *buf, int size);

/* allows the caller to explicitly lock the given operations. Blocks until the
 * locks are acquired. The caller needs to make sure `dsUnlock` for the same
 * operations in order to avoid rendering the data structure unusable.
 *
 * Returns -1 on error. */
int dsLock(void *mem, unsigned char operations);

/* unlocks the given operations on the block of memory passed. See documentation
 * for `dsLock`.
 *
 * Returns -1 on error */
int dsUnlock(void *mem, unsigned char operations);

/* deletes the entry associated with the given name.
 *
 * Returns -1 on error. When the name is not found, errno is set to EINVAL */
int dsDelete(void *mem, char *name);

/* destroys the data structure and associated resources
 *
 * This method needs to be called *before* the `mem` block is released back to
 * the system, and only when there is no intention of using the same block of
 * memory in the future.
 */
void dsDestroy(void *mem);

#endif /* DS_H */
