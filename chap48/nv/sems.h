/* sems.h - implementation of a simple, semaphore-based synchronization mechanism.
*
* This defines an API of a simple binary semaphore, allowing callers to either
* reserve or release a resource. The underlying mechanism to provide the resource
* synchronization is System V semaphores.
*
* Author: Renato Mascarenhas Costa
*/

#ifndef SEMS_H
#define SEMS_H

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <errno.h>

/* whether a newly created semaphore is available or in use. Values are arbitrary */
#define SEM_AVAILBLE (0)
#define SEM_IN_USE   (1)

/* permissions of the underlying System V semaphore */
#define SEMAPHORE_PERMS (S_IRUSR | S_IWUSR)

/* define mandatory union according to SUSv3 (for portability)
 *
 * OS X's sys/sem.h file already has this union defined, so we skip this definition
 * on Apple computers */
#if !defined(__APPLE__)
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
#if defined(__linux__)
	struct seminfo *__buf;
#endif /* __linux__ */
};
#endif /* __APPLE__ */

/* creates a new binary semaphore in the state given (either SEM_AVAILBLE or
 * SEM_IN_USE.) Returns the semaphore identifier on success, or -1 on error.
 *
 * Future calls to semaphore related functions must pass the identifier returned
 * on this function. */
int semaphoreInit(int state);

/* checks the state of the semaphore with the given identifier.
 *
 * Returns -1 on error */
int semaphoreGetState(int semid);

/* Reserves the semaphore with the identifier given. Blocks if the semaphore is
 * already reserved until it becomes available. Returns -1 on error. */
int semaphoreReserve(int semid);

/* Release the semaphore with the identifier given. Returns -1 on error */
int semaphoreRelease(int semid);

/* Destroys associated resources for a semaphore with the identifier given */
void semaphoreDestroy(int semid);

#endif /* SEMS_H */
