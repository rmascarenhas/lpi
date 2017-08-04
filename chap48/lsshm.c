/* lsshm.c - Lists all System V shared memory segments on the running system.
 *
 * System V shared memory segments allow fast inter process communication, providing
 * direct access bu binding the same physical address space to multiple processes'
 * virtual address spaces. However, for its simplicity, it delegates some of the
 * complexity to the callers: for instance, access synchronization must be done
 * explicitly, maybe by using semaphores.
 *
 * This Linux-specific program lists all System V shared memory segments on the
 * running system.
 *
 * Usage
 *
 *    $ ./lsshm
 *
 * Author: Renato Mascarenhas Costa
 */

/* get definition of Linux-specific `shmctl(2)` operations (SHM_INFO and SHM_STAT) */
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static void pexit(const char *fCall);

int
main() {
	int i, maxind, shmid;
	struct shmid_ds ds;
	struct shm_info info;

	maxind = shmctl(0, SHM_INFO, (struct shmid_ds *) &info);
	if (maxind == -1)
		pexit("shmctl");

	printf("Total shared memory segments: %d\n", info.used_ids);
	printf("Number of memory pages these occupy: %ld\n", info.shm_tot);

	printf("\n%10s\t%10s\t%10s\t%10s\n", "ID", "key", "size (b)", "processes");

	for (i = 0; i <= maxind; i++) {
		/* first argument is an array index when using SHM_STAT */
		shmid = shmctl(i, SHM_STAT, &ds);

		if (shmid == -1) {
			/* EINVAL and EACCES likely returned multiple times when checking for
			 * non-existent indexes from 0 up to the maximum index. */
			if (errno != EINVAL && errno != EACCES)
					pexit("semctl");

			continue;
		}

		printf("%10d\t%10ld\t%10ld\t%10ld\n", shmid, (long) ds.shm_perm.__key, (long) ds.shm_segsz, (long) ds.shm_nattch);
	}

	printf("\n");

	exit(EXIT_SUCCESS);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
