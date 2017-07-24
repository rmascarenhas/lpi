/* common.h - Common definitions for the client and server programs of _talk.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TALK_CONN_DIR   "/tmp/.talkd"          /* manage connections under this path */
#define SERVER_QID_PATH (TALK_CONN_DIR "/key") /* path to the file containing the server's queue identifier */

#define DATA_SIZE (1024) /* use a static sized buffer when transmitting messages */
#define MAX_SV_QUEUE_ID_LEN (32) /* System V message queue ID should not be longer than 32 characters */

struct requestMsg {                    /* connection requests: from clients to the server */
	long mtype;                        /* message type - always TALK_MT_REQ_CONNECT */
	int clientId;                      /* the message queue ID of the client process */
	char fromUsername[LOGIN_NAME_MAX]; /* the username of the requesting user */
	char toUsername[LOGIN_NAME_MAX];   /* the username of the intended recipient of the message */
	char data[DATA_SIZE];              /* data field - includes message content when exchanging messages */
};

/* take possible padding among fields when calculating the message size */
#define TALK_REQ_MSG_SIZE (offsetof(struct requestMsg, data) - offsetof(struct requestMsg, clientId) + DATA_SIZE)

struct responseMsg {    /* connection responses: from the server to clients */
	long mtype;           /* message type: one of TALK_MT_RES_CONNECT_* */
	char data[DATA_SIZE]; /* in case connection failure, this field includes more information */
};

/* only the data field is included */
#define TALK_RES_MSG_SIZE (DATA_SIZE)

#define TALK_MT_REQ_CONNECT         (1)  /* client requests connection */
#define TALK_MT_RES_CONNECT_ACCEPT  (2)  /* client accepts connection */
#define TALK_MT_RES_CONNECT_FAILURE (3)  /* connection failed */
#define TALK_MT_REQ_TALK_MSG        (4)  /* client sends a message to another client */
#define TALK_MT_REQ_TALK_CONN_DROP  (6)  /* one client over a communication has disconnected */

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
