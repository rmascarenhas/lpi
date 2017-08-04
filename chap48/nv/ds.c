/* ds.c - implementation of a simple key/value data structure.
*
* This implements the functions defined on the `ds.h` file. It provides an API
* like that of a hashtable or dictionary. The underlying implementation, however,
* uses a naive array with linear lookup and insertion time for simplicity.
*
* Author: Renato Mascarenhas Costa
*/

#include "ds.h"

/* NOTE: concurrent access to the block of memory managed by this library is
 * not thread safe. However, read and write locks are kept in its metadata,
 * and an API to lock and unlock them is provided (see `dsLock` and `dsUnlock`.)
 *
 * Therefore, it is a responsibility of the caller to appropriately lock the
 * data structure according to the needs to the application. */

static void
setSize(void *mem, int size) {
	int *p = mem;
	p[DS_SIZE_OFFSET] = size;
}

static int
getSize(void *mem) {
	int *p = mem;
	return p[DS_SIZE_OFFSET];
}

static void
setCap(void *mem, int cap) {
	int *p = mem;
	p[DS_CAP_OFFSET] = cap;
}

static int
getCap(void *mem) {
	int *p = mem;
	return p[DS_CAP_OFFSET];
}

static void
setReadLock(void *mem, int semid) {
	int *p = mem;
	p[DS_RLOCK_OFFSET] = semid;
}

static int
getReadLock(void *mem) {
	int *p = mem;
	return p[DS_RLOCK_OFFSET];
}

static void
setWriteLock(void *mem, int semid) {
	int *p = mem;
	p[DS_WLOCK_OFFSET] = semid;
}

static int
getWriteLock(void *mem) {
	int *p = mem;
	return p[DS_WLOCK_OFFSET];
}

static void *
arrayAddr(void *mem) {
	int *p = mem;
	return p + 4;
}

int
dsInit(void *mem, int cap) {
	int rsemid, wsemid;

	rsemid = semaphoreInit(SEM_AVAILBLE);
	if (rsemid == -1)
		return -1;

	wsemid = semaphoreInit(SEM_AVAILBLE);
	if (wsemid == -1)
		return -1;

	setSize(mem, 0);
	setCap(mem, cap);
	setReadLock(mem, rsemid);
	setWriteLock(mem, wsemid);

	return 0;
}

int
dsValidate(void *mem) {
	int size, cap, rsemid, wsemid;
	bool validReadLock, validWriteLock;

	size = getSize(mem);
	cap = getCap(mem);
	rsemid = getReadLock(mem);
	wsemid = getWriteLock(mem);

	/* checks if the semaphore IDs on the metadata are valid System V semaphores:
	 * they might have been wiped out since they were created */
	validReadLock = (semaphoreGetState(rsemid) != -1);
	validWriteLock = (semaphoreGetState(wsemid) != -1);

	if (size >= 0 && size <= cap && validReadLock && validWriteLock)
		return 0;

	/* corrupted or invalid block of memory */
	return -1;
}

int
dsCapToBytes(int cap) {
	return (4 * sizeof(int)) + (cap * sizeof(struct nvds_entry));
}

int
dsSet(void *mem, char *name, char *val) {
	int i, size, cap;
	struct nvds_entry *array;

	/* make sure parameters given are within accepted limits */
	if (strlen(name) > NVDS_NAME_LEN || strlen(val) > NVDS_VAL_LEN) {
		errno = EINVAL;
		return -1;
	}

	size = getSize(mem);
	cap = getCap(mem);
	array = arrayAddr(mem);

	for (i = 0; i < size; i++) {
		/* check for an existing entry with the given name: if it exists, update it
		 * and return successfully */
		if (strncmp(array[i].name, name, NVDS_NAME_LEN) == 0) {
			strncpy(array[i].val, val, NVDS_VAL_LEN);
			return 0;
		}
	}

	/* the name requested does not exist - insert it */

	/* if there is no more capacity for insertion, end with ENOMEM */
	if (size >= cap) {
		errno = ENOMEM;
		return -1;
	}

	/* copy the value given to the end of the list and increase the size metadata */
	strncpy(array[size].name, name, NVDS_NAME_LEN);
	strncpy(array[size].val, val, NVDS_VAL_LEN);
	setSize(mem, size + 1);

	return 0;
}

int
dsGet(void *mem, char *name, char *buf, int bufsiz) {
	int i, size, namelen;
	struct nvds_entry *array;

	if (strlen(name) > NVDS_NAME_LEN) {
		errno = EINVAL;
		return -1;
	}

	size = getSize(mem);
	array = arrayAddr(mem);
	namelen = strlen(name);

	for (i = 0; i < size; i++) {
		if (strncmp(array[i].name, name, namelen) == 0) {
			/* name found - copy value to the provided buffer */
			strncpy(buf, array[i].val, bufsiz);
			return 0;
		}
	}

	/* name not found: return EINVAL to the caller */
	errno = EINVAL;
	return -1;
}

int
dsLock(void *mem, unsigned char operations) {
	int rsemid, wsemid;

	rsemid = getReadLock(mem);
	wsemid = getWriteLock(mem);

	if (operations & DS_READ) {
		if (semaphoreReserve(rsemid) == -1)
			return -1;
	}

	if (operations & DS_WRITE) {
		if (semaphoreReserve(wsemid) == -1)
			return -1;
	}

	/* requested locks acquired successfully */
	return 0;
}

int dsUnlock(void *mem, unsigned char operations) {
	int rsemid, wsemid;

	rsemid = getReadLock(mem);
	wsemid = getWriteLock(mem);

	if (operations & DS_WRITE) {
		if (semaphoreRelease(wsemid) == -1)
			return -1;
	}

	if (operations & DS_READ) {
		if (semaphoreRelease(rsemid) == -1)
			return -1;
	}

	/* requested locks released successfully */
	return 0;
}

int
dsDelete(void *mem, char *name) {
	int i, j, size;
	struct nvds_entry *array;

	if (strlen(name) > NVDS_NAME_LEN) {
		errno = EINVAL;
		return -1;
	}

	size = getSize(mem);
	array = arrayAddr(mem);

	for (i = 0; i < size; i++) {
		if (strncmp(array[i].name, name, NVDS_NAME_LEN) == 0) {
			/* requested name found - shift other elements left */
			for (j = i; j < size - 1; j++) {
				array[j] = array[j + 1];
			}

			/* update size metadata */
			setSize(mem, size - 1);
			return 0;
		}
	}

	/* element not found: return error to the caller */
	errno = EINVAL;
	return -1;
}

void
dsDestroy(void *mem) {
	int rsemid, wsemid;

	rsemid = getReadLock(mem);
	wsemid = getWriteLock(mem);

	semaphoreDestroy(rsemid);
	semaphoreDestroy(wsemid);
}
