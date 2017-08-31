/* client.c - Client side of the message queue file server.
 *
 * This implements the client of a simple file server that uses POSIX message
 * queues as a means of interprocess communication. When invoked via the command
 * line, it sends a message to the server and waits for a response back. If
 * successful, the contents of the requested file are printed on the standard
 * output.
 *
 * Usage
 *
 *    $ ./client [path]
 *    path - path to the file to be requested to the server
 *
 * Author: Renato Mascarenhas Costa
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* maintain a copy of the temporary POSIX message queue name to be used by the
 * client so that the 'cleanup' function can later delete it when the program
 * terminates or receives a signal */
char cmqname[NAME_MAX];
mqd_t cmqdes;

static void sighandler(int signal);
static void cleanup(void);
static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
	if (argc != 2)
		helpAndExit(argv[0], EXIT_FAILURE);

	mqd_t smqdes;
	struct mq_attr attr;
	struct reqmsg request;
	struct respmsg *response;
	struct sigaction act;
	bool fin;

	/* steps to request a file to the server:
	 *
	 * 1. open the server message queue
	 * 2. create a temporary message queue to receive a response from the server
	 * 3. make sure the temporary message queue is delete on program termination
	 * 4. send a request to the server for the requested path
	 * 5. wait for a response from the server on the queue created on step 2
	 * 6. loop until MSG_FIN is received, printing the received data to the standard output
	 * 7. close both queues
	 */

	/* 1 */
	smqdes = mq_open(SERVER_MQNAME, O_WRONLY);
	if (smqdes == (mqd_t) -1)
		pexit("mq_open");

	/* 2 */
	snprintf(request.mqname, NAME_MAX, "/mqfs-client-%ld", (long) time(NULL));
	strncpy(cmqname, request.mqname, NAME_MAX);
	setQueueAttributes(&attr);

	cmqdes = mq_open(request.mqname, O_RDONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
	if (cmqdes == (mqd_t) -1)
		pexit("mq_open");

	/* 3 */
	if (atexit(cleanup) != 0)
		pexit("atexit");

	act.sa_handler = sighandler;
	if (sigaction(SIGINT, &act, NULL) == -1)
		pexit("sigaction");

	if (sigaction(SIGTERM, &act, NULL) == -1)
		pexit("sigaction");

	/* 4 */
	strncpy(request.pathname, argv[1], NAME_MAX);
	if (mq_send(smqdes, (const char *) &request, sizeof(struct reqmsg), 0) == -1)
		pexit("mq_send");

	/* 5 and 6 */
	fin = false;
	while (!fin) {
		response = malloc(MSG_LEN);
		if (response == NULL)
			pexit("malloc");

		if (mq_receive(cmqdes, (char *) response, MSG_LEN, NULL) == -1)
			pexit("mq_receive");

		switch (response->mtype) {
			case MSG_FAILURE:
				fprintf(stderr, "Failure: %s\n", response->data);
				break;

			case MSG_DATA:
				printf(response->data);
				break;

			case MSG_FIN:
				fin = true;
				break;

			default:
				fprintf(stderr, "Panic: unrecognized message type: %d\n", response->mtype);
				exit(EXIT_FAILURE);
		}

		free(response);
	}

	/* 7 */
	if (mq_close(smqdes) == -1)
		pexit("mq_close");

	if (mq_close(cmqdes) == -1)
		pexit("mq_close");

	if (mq_unlink(request.mqname) == -1)
		pexit("mq_unlink");

	exit(EXIT_SUCCESS);
}

static void
sighandler(__attribute__((unused)) int signal) {
	exit(EXIT_SUCCESS); /* properly invokes atexit(3) handlers */
}

static void
cleanup() {
	/* when the process is about to terminate, unlink the temporary message queue, so that
	 * no resources will be leaked */
	mq_close(cmqdes);
	mq_unlink(cmqname);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [path]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
