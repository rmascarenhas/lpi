/* server.c - Server side of the message queue file server.
 *
 * This implements the server end of the 'mqfs' tool - a simple file server
 * using POSIX message queues for IPC. The server creates a new message queue
 * named as defined in +SERVER_MQNAME+ and waits for messages from clients.
 *
 * For each file received, the server will attempt to read the file, and send
 * its contents back to the requester in chunks of +RESP_BUFFER_SIZE+ bytes.
 * Failures are also sent back to the caller appropriately.
 *
 * Usage
 *
 *    $ ./server
 *
 * Once invoked, the server proces will block indefinitely waiting for incoming
 * messages. SIGINT/SIGTERM can be used to terminate the server, which will clean
 * up after itself before exiting.
 *
 * Author: Renato Mascarenhas Costa
 */

#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* maximum length a line of logs may have */
#define LOGLEN (1024)

static void pexit(const char *fCall);
static void sighandler(int signal);
static void cleanup(void);

static void warning(const char *message);
static int sendResponse(mqd_t mqdes, struct reqmsg *request, struct respmsg *response);

int
main() {
	struct reqmsg *request;
	struct respmsg response;
	struct mq_attr attr;
	struct sigaction act;
	mqd_t smqdes, cmqdes;
	int fd;
	ssize_t numRead;
	char buf[LOGLEN];

	/* the server loop:
	 *
	 * 1. create the server message queue, which should not exist (precondition)
	 * 2. make sure to invoke the 'cleanup' function if interrupted by a signal or terminated
	 * 3. wait for incoming messages
	 * 4. when message arrives, try to open the requested file and the client's message queue
	 * 5. for each chunk of RESP_BUFFER_SIZE bytes, send a MSG_DATA back to the caller
	 * 6. if there is a failure finding or reading the file, report the error back with MSG_FAILURE
	 * 7. Send MSG_FIN to the caller to indicate processing is done
	 * 8. close the client's message queue
	 */

	/* 1 */
	setQueueAttributes(&attr);
	smqdes = mq_open(SERVER_MQNAME, O_CREAT | O_EXCL | O_RDONLY, S_IRUSR | S_IWUSR, &attr);
	if (smqdes == (mqd_t) -1)
		pexit("mq_open");

	/* 2 */
	if (atexit(cleanup) != 0)
		pexit("atexit");

	act.sa_handler = sighandler;
	if (sigaction(SIGINT, &act, NULL) == -1)
		pexit("sigaction");
	if (sigaction(SIGTERM, &act, NULL) == -1)
		pexit("sigaction");

	/* if, at any point, communicating with the client queue fails, a message is
	* logged to the server's standard error, and we proceed to wait for the next
	* request */
	for (;;) {
		/* 3 */
		request = malloc(MSG_LEN);
		if (request == NULL)
			pexit("malloc");

		if (mq_receive(smqdes, (char *) request, MSG_LEN, NULL) == -1)
			pexit("mq_receive");

		/* 4 */
		cmqdes = mq_open(request->mqname, O_WRONLY);
		if (cmqdes == (mqd_t) -1) {
			/* failed to open client message queue - perhaps client died? Log message
			 * and proceed */
			snprintf(buf, LOGLEN, "failed to open client's message queue at %s: %s", request->mqname, strerror(errno));
			warning(buf);
			continue;
		}

		fd = open(request->pathname, O_RDONLY);
		if (fd == -1) {
			/* failed to open the requested file - maybe it does not exist, or the user
			 * does not have permissions - in either way, send a MSG_FAILURE to indicate
			 * the situation followed by MSG_FIN since the server will move on to the next
			 * client */
			response.mtype = MSG_FAILURE;
			strncpy(response.data, strerror(errno), RESP_BUFFER_SIZE);
			if (sendResponse(cmqdes, request, &response) == -1)
				continue;

			response.mtype = MSG_FIN;
			response.data[0] = '\0';
			if (sendResponse(cmqdes, request, &response) == -1)
				continue;

			mq_close(cmqdes);
			continue;
		}

		while ((numRead = read(fd, response.data, RESP_BUFFER_SIZE - 1)) > 0) {
			/* 5 */
			response.mtype = MSG_DATA;
			response.data[numRead] = '\0';

			if (sendResponse(cmqdes, request, &response) == -1)
				break;
		}

			/* 6 */
		if (numRead == -1) {
			response.mtype = MSG_FAILURE;
			strncpy(response.data, strerror(errno), RESP_BUFFER_SIZE);

			if (sendResponse(cmqdes, request, &response) == -1)
				continue;
		}

		/* 7 */
		response.mtype = MSG_FIN;
		response.data[0] = '\0';

		if (sendResponse(cmqdes, request, &response) == -1)
			continue;

		mq_close(cmqdes);
		free(request);
	}

	exit(EXIT_SUCCESS); /* unreachable */
}

static int
sendResponse(mqd_t mqdes, struct reqmsg *request, struct respmsg *response) {
	char buf[LOGLEN];
	char *mtype = response->mtype == MSG_FAILURE ? "MSG_FAILURE" :
		response->mtype == MSG_DATA ? "MSG_DATA" : "MSG_FIN";

	if (mq_send(mqdes, (const char *) response, sizeof(struct respmsg), 0) == -1) {
		snprintf(buf, LOGLEN, "failed to send message %s to client at %s (%s): %s",
			mtype, request->mqname, request->pathname, strerror(errno));
		warning(buf);

		return -1;
	}

	return 0;
}

static void
warning(const char *message) {
	fprintf(stderr, "[server] %s\n", message);
}

static void
sighandler(__attribute__((unused)) int signal) {
	exit(EXIT_SUCCESS); /* properly invokes atexit(3) handlers */
}

static void
cleanup() {
	/* when the process is about to terminate, unlink the server message queue, so that
	 * subsequent invokations of the server will work properly */
	mq_unlink(SERVER_MQNAME);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
