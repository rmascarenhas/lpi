/* ef.c - An implementation of VMS' event flags on top of System V Semaphores.
 *
 * This implements the library functions for the event flags implementation. See
 * the ef.h file for more information.
 *
 * Author: Renato Mascarenhas Costa
 */

#include "ef.h"

int
efCreate(int state) {
	union semun arg;
	int semId;

	if (state == EF_SET || state == EF_CLEAR) {
		arg.val = state;
	} else {
		errno = EINVAL;
		return -1;
	}

	/* create a semaphore set with 1 semaphore - always manipulated individually */
	semId = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	if (semId == -1) {
		return -1;
	}

	/* initialize the semaphore with the state chosen */
	if (semctl(semId, 0, SETVAL, arg) == -1) {
		return -1;
	}

	return semId;
}

int
efSet(int id) {
	/* perform one operation - decrease the underlying semaphore's value by 1 */
	struct sembuf sop;

	sop.sem_num = 0; /* semaphore sets handled here always have only one semaphore */
	sop.sem_op = -1;
	sop.sem_flg = efUseSemUndo ? SEM_UNDO : 0;

	while (semop(id, &sop, 1) == -1)
		if (errno != EINTR || !efRetryOnEintr)
			return -1;

	return 0;
}

int
efClear(int id) {
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_op = 1;
	sop.sem_flg = efUseSemUndo ? SEM_UNDO : 0;

	return semop(id, &sop, 1); /* does not block - incrementing the semaphore's value */
}

int
efGet(int id) {
	union semun dummy; /* to make standards happy */
	return semctl(id, 0, GETVAL, dummy);
}

int
efWait(int id) {
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_op = 0; /* wait for the semaphore's value to become 0 (set) */
	sop.sem_flg = efUseSemUndo ? SEM_UNDO : 0;

	while (semop(id, &sop, 1) == -1)
		if (errno != EINTR || !efRetryOnEintr)
			return -1;

	return 0;
}

int
efDestroy(int id) {
	union semun dummy;
	return semctl(id, 0, IPC_RMID, dummy);
}
