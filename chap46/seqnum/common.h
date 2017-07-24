/* common.h - Common definitions for the client and server programs.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* the fixed key to be used for the message queue where server-client communication
 * will happen */
#define MSGQ_KEY (0x1aaaaaa1)

/* server only reads messages of this type written to the queue. */
#define SERVER_MSG_TYPE (1) /* use PID of init, which will never be a client process */

struct requestMsg {
	long mtype; /* will always be SERVER_MSG_TYPE */
	pid_t pid;  /* will include the process ID of the client process */
	int seqLen; /* how many instances to "allocate" from the sequence */
};

/* account for possible padding between pid and seqLen */
#define REQ_MSG_LEN (offsetof(struct requestMsg, seqLen) - offsetof(struct requestMsg, pid) + sizeof(int))

struct responseMsg {
	long mtype; /* will always be the PID of the client process */
	int seqNum; /* the start index of the "allocated" sequence */
};
#define RESP_MSG_LEN (sizeof(int)) /* we are only sending the seqNum field on our responses */

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
