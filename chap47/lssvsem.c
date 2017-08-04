/* lssvsem.c - Lists all System V semaphores active on the running system.
 *
 * System V semaphores allow callers to atomically increment/decrement a counter
 * managed by the kernel. However, the management of semaphores happen in _sets_:
 * a list of operations to be performed on a set of semaphores is guaranteed to be
 * atomic.
 *
 * This Linux-specific program lists all System V semaphore sets active on the
 * running system.
 *
 * Usage
 *
 *    $ ./lssvsem
 *
 * Author: Renato Mascarenhas Costa
 */

/* get definition of Linux-specific `semctl(2)` operations */
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

/* define this union as required by the standards */
union semun {
		int val;
		struct semid_ds *buf;
		unsigned short *array;
#if defined(__linux__)
		struct seminfo *__buf;
#endif
};

static void pexit(const char *fCall);

int
main() {
	int i, maxind, semid;
	struct semid_ds ds;
	struct seminfo info;

	maxind = semctl(0, 0, SEM_INFO, (struct semid_ds *) &info);
	if (maxind == -1) {
		pexit("semctl");
	}

	printf("Maximum index in the kernel's array: %d\n", maxind);
	printf("Total semaphore sets: %d\n", info.semusz);
	printf("Total semaphores: %d\n", info.semaem);

	printf("\n%10s\t%10s\t%10s\t%10s\n", "index", "ID", "key", "semaphores");

	for (i = 0; i <= maxind; i++) {
		semid = semctl(i, 0, SEM_STAT, &ds);
		if (semid == -1) {
			/* EINVAL and EACCES likely returned multiple times when checking for
			 * non-existent indexes from 0 up to the maximum index. */
			if (errno != EINVAL && errno != EACCES)
					pexit("semctl");

			continue;
		}

		printf("%10d\t%8d\t0x%08lx\t%7ld\n", i, semid, (unsigned long) ds.sem_perm.__key, (long) ds.sem_nsems);
	}

	exit(EXIT_SUCCESS);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
