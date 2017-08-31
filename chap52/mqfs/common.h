/* common.h - common header for client and server of the message queue file server (mqfs)
*
* This includes function definitions and common data structures to be used by both the client
* and the server of the mqfs program.
*
* 'mqfs' is a simple file server that uses POSIX message queues in order to communicate - in
* other words, both processes need to reside in the same host. See documentation on the
* client.c and server.c files for usage instructions of each of these components.
*
* Author: Renato Mascarenhas Costa
*/

#ifndef COMMON_H
#define COMMON_H

#include <mqueue.h>
#include <signal.h>
#include <limits.h>
#include <stdbool.h>

#ifndef RESP_BUFFER_SIZE /* allow the compiler to override the default */
#	define RESP_BUFFER_SIZE (1024)
#endif

#ifndef SERVER_MQNAME
#	define SERVER_MQNAME ("/mqfs-server")
#endif

/* make definition of the queue's message length independent of the
 * actual implementation of the structs by using the largest, and
 * casting appropriately */
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MSG_LEN (MAX(sizeof(struct reqmsg), sizeof(struct respmsg)))

#define MSG_FAILURE (0) /* there was a failure fulfilling the request */
#define MSG_DATA    (1) /* data message - file content attached */
#define MSG_FIN     (2) /* all the content of the requested file has been transferred */

struct reqmsg {
	char pathname[NAME_MAX]; /* path to the requested file */
	char mqname[NAME_MAX];   /* name of the queue where the client will wait for a response */
};

struct respmsg {
	int mtype;                   /* message type - see definition os MSG_* constants */
	char data[RESP_BUFFER_SIZE]; /* file content (MSG_DATA); or error message (MSG_FAILURE) */
};

/* sets POSIX message queue attributes to consistent default values across client
 * and server to avoid having to rely on implementation specific defaults */
void setQueueAttributes(struct mq_attr *attr);

#endif /* COMMON_H */
