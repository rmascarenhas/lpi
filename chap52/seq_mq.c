/* seq_mq.c - A sequence server using POSIX message queues.
 *
 * This implements a simple "sequence" server, which accepts messages from
 * clients and returns a sequential identifier to each of them. Used to
 * demonstrate the POSIX message queue API (the target UNIX implementation is Linux
 * but effort has been made to improve portability, so this program should work
 * on other implementations with few modifications, if any.)
 *
 * Usage
 *
 *    $ ./seq_mq [-c] [-s] [name]
 *    -c:   creates a new client process. The client will send a message to the
 *          message queue identified by the 'name' parameter, and print the sequence
 *          number received back.
 *    -s:   creates a new server process. The message queue with 'name' name given
 *          is created, and the process blocks waiting for client messages.
 *    name: the unique identifier of the message queue to be created/read from.
 *          Needs to conform to the POSIX IPC naming conventions.
 *
 * When compiling, this program needs to be linked with the realtime library using
 * -lrt.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE /* get definition of getopt(3) */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* make definition of message length independent of the actual implementation
 * of the structs by using the largest, and casting appropriately */
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MSG_LEN (MAX(sizeof(struct reqmsg), sizeof(struct respmsg)))

static void cleanup(int signal);
static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

static void client(char *mqname);
static void server(char *mqname);

/* manage the sequence to be returned to each client using a global variable */
long counter = 0;

/* stores the server message queue name in a global variable so that it is
 * accessible by the `cleanup` function */
char servermqname[NAME_MAX];

/* request message: when the client requests a sequence number to the server,
 * it indicates the queue name in which the client will be waiting for a response. */
struct reqmsg {
	char mqname[NAME_MAX];
};

struct respmsg {
	long seq; /* the sequence number returned by the server on a request by a client */
};

int
main(int argc, char *argv[]) {
	if (argc != 3)
		helpAndExit(argv[0], EXIT_FAILURE);

	char opt;
	bool isClient = false;
	struct sigaction act;

	while ((opt = getopt(argc, argv, "cs")) != -1) {
		switch (opt) {
			case 'c':
				isClient = true;
				break;

			case 's':
				isClient = false;
				break;

			default:
				fprintf(stderr, "Invalid option: %c\n", opt);
				helpAndExit(argv[0], EXIT_FAILURE);
		}
	}

	strncpy(servermqname, argv[2], NAME_MAX);

	/* make sure that the server's message queue is removed when the server
	* process terminates (i.e., is interrupted by the user via a Ctrl-C) */
	if (!isClient) {
		act.sa_handler = cleanup;

		if (sigaction(SIGINT, &act, NULL) == -1)
			pexit("sigaction");

		if (sigaction(SIGTERM, &act, NULL) == -1)
			pexit("sigaction");
	}

	if (isClient)
		client(argv[2]);
	else
		server(argv[2]);

	exit(EXIT_SUCCESS);
}

/* set message queue default attributes, to avoid having to rely on
 * implementation-dependent values */
static void
setDefaultAttributes(struct mq_attr *attr) {
	attr->mq_flags = 0;
	attr->mq_maxmsg = 10;
	attr->mq_msgsize = MSG_LEN;
}

static void
client(char *mqname) {
	mqd_t clientmqdes, servermqdes;
	struct reqmsg request;
	struct respmsg *response;
	struct mq_attr attr;

	snprintf(request.mqname, NAME_MAX, "/seq_mq_cl-%ld", (long) time(NULL));
	setDefaultAttributes(&attr);

	/* buffer allocated to receive the response must be at least equal to the 'msgsize'
	 * attribute of the queue */
	response = malloc(MSG_LEN);
	if (response == NULL)
		pexit("malloc");

	/* create the queue on which the client will wait for a response from the
	 * server - make sure the queue is created and it did not exist before */
	clientmqdes = mq_open(request.mqname, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
	if (clientmqdes == (mqd_t) -1)
		pexit("mq_open");

	/* open the server message queue for writing - we need to add our request
	 * to the queue */
	servermqdes = mq_open(mqname, O_WRONLY);
	if (servermqdes == (mqd_t) -1)
		pexit("mq_open");

	/* send request passing the queue in which we will wait for a response */
	printf("Sending request to server...\n");
	if (mq_send(servermqdes, (const char *) &request, sizeof(struct reqmsg), 0) == -1)
		pexit("mq_send");

	if (mq_receive(clientmqdes, (char *) response, MSG_LEN, NULL) == -1)
		pexit("mq_receive");

	printf("Server replied: %ld\n", response->seq);

	/* close queues and mark client queue for destruction */
	if (mq_close(servermqdes) == -1)
		pexit("mq_close");

	if (mq_close(clientmqdes) == -1)
		pexit("mq_close");

	if (mq_unlink(request.mqname) == -1)
		pexit("mq_unlink");

	free(response);
}

static void
server(char *mqname) {
	mqd_t servermqdes, clientmqdes;
	struct reqmsg *request;
	struct respmsg response;
	struct mq_attr attr;

	setDefaultAttributes(&attr);

	request = malloc(MSG_LEN);
	if (request == NULL)
		pexit("malloc");

	/* first, make sure we are creating a new message queue with the identifier
	 * given */
	servermqdes = mq_open(mqname, O_RDONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
	if (servermqdes == (mqd_t) -1)
		pexit("mq_open");

	for (;;) {
		if (mq_receive(servermqdes, (char *) request, MSG_LEN, NULL) == -1)
			pexit("mq_receive");

		/* open queue in which requester will be waiting for a response */
		clientmqdes = mq_open(request->mqname, O_WRONLY);
		if (clientmqdes == (mqd_t) -1)
			pexit("mq_open");

		counter++;
		response.seq = counter;

		/* send the response back to the caller */
		if (mq_send(clientmqdes, (const char *) &response, sizeof(struct respmsg), 0) == -1)
			pexit("msg_send");

		/* response sent. Close the no longer needed client queue */
		if (mq_close(clientmqdes) == -1)
			pexit("mq_close");
	}
}

static void
cleanup(__attribute__((unused)) int signal) {
	if (strlen(servermqname) > 0) {
		mq_unlink(servermqname);
	}

	exit(EXIT_SUCCESS);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [-c] [-s] [name]\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
