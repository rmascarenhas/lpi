/* ef.h - An implementation of VMS' event flags on top of System V Semaphores.
 *
 * System V's semaphores are notorious for its inherent complexity: semaphores can hold
 * any value larger than 0 (up to a maximum); they are special entities, as opposed to regular
 * Unix files; and, most importantly for this discussion, can only be manipulated in *sets*.
 * However, most times applications just need to manipulate semaphores individually and having
 * a set-based API makes things more complicated than necessary.
 *
 * This implements the idea of *event flags* from the VMS operating system on top of System V's
 * semaphores. Event flags binary semaphores and are either *clear* or *set*. Process can set
 * event flags; clear event flags; and conditionally set event flags.
 *
 * Author: Renato Mascarenhas Costa
 */

#ifndef EF_H
#define EF_H

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>

#include <errno.h>

#include <stdbool.h>

/* counter intuitively, an event flag that is set has the value of 0 and is clear when it is equal
 * to one. That is convenient since System V semaphore primitives work by waiting a semaphore to
 * become `0`. */
#define EF_SET   (0)
#define EF_CLEAR (1)

extern bool efUseSemUndo;   /* use SEM_UNDO when creating the underlying SV semaphore    */
extern bool efRetryOnEintr; /* retry if a blocked system call is interrupted by a signal */

/* define mandatory union as defined by SUSv3 */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
#if defined(__linux__)
	struct seminfo *__buf;
#endif
};

/* creates a new event flag, and returns an identifier that needs to be passed on future
 * calls to functions of this library. The `state` parameter indicates whether the new
 * event flag should be set (EF_SET) or clear (EF_CLEAR).
 *
 * If the global `efUseSemUndo` variable is set, the semantics of the SV's SEM_UNDO flag
 * are applied. */
int efCreate(int state);

/* sets an event flag with the identifier given. If the event flag is already set, this
 * call will block until the flag can be set. Behaviour of this call when it is interrupted
 * by a signal can be controlled by the `efRetryOnEintr` global */
int efSet(int id);

/* clears the event flag with the identifier provided */
int efClear(int id);

/* returns the state of the event flag: either EF_SET or EF_CLEAR */
int efGet(int id);

/* waits for the event flag to be set. If it is not currently set, this call will
 * block until it is */
int efWait(int id);

/* removes an event flag, destroying the underlying System V semaphore associated with it */
int efDestroy(int id);

#endif /* EF_H */
