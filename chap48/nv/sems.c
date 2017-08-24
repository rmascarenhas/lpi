/* sems.c - implementation of a simple, semaphore-based synchronization mechanism.
*
* This implements the functions defined on the `sems.h` file. It provides an API
* that allow callers to create, reserve and release a binary semaphore, to be used
* as a synchronization mechanism for a shared resource.
*
* Author: Renato Mascarenhas Costa
*/

#include "sems.h"

int
semaphoreInit(int state) {
	int val, semid;

	if (state == SEM_AVAILBLE) {
		val = 1;
	} else if (state == SEM_IN_USE) {
		val = 0;
	} else {
		errno = EINVAL;
		return -1;
	}

	semid = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | SEMAPHORE_PERMS);
	if (semid == -1)
		return -1;

	union semun arg;
	arg.val = val;

	if (semctl(semid, 0, SETVAL, arg) == -1)
		return -1;

	return semid;
}

int
semaphoreGetState(int semid) {
	union semun dummy = {};
	return semctl(semid, 0, GETVAL, dummy);
}

int
_semop(int semid, int operation) {
	struct sembuf semops;

	semops.sem_num = 0;
	semops.sem_op = operation;

	return semop(semid, &semops, 1);
}

int
semaphoreReserve(int semid) {
	return _semop(semid, -1);
}

int
semaphoreRelease(int semid) {
	return _semop(semid, 1);
}

void
semaphoreDestroy(int semid) {
	union semun dummy = { 0 }; /* portability */

	semctl(semid, 0, IPC_RMID, dummy);
}
