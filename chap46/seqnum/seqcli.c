/* seqcli.c - A simple sequence server using System V message queues (client).
 *
 * Client program that communicates with the server (see seqser.c.) This program
 * reads the size of the request to be made (how many units in the server's sequence
 * to be "allocated"), builds the corresponding message to the server, writes it to
 * message queue, reads the response back, and prints it on the standard output.
 *
 * Based on similar program included in The Linux Programming Interface book.
 *
 * Usage:
 *
 *   $ ./seqcli seqLen
 *   seqLen: an integer, number of resources to be allocated from the server's sequence.
 *
 * Author: Renato Mascarenhas Costa
 */

#include "common.h"
#include <unistd.h>
#include <limits.h>

static void helpAndExit(const char *progname, int status);

int
main(int argc, char *argv[]) {
	if (argc != 2)
		helpAndExit(argv[0], EXIT_FAILURE);

	char *endptr;
	long length;
	int msgqid;
	pid_t pid;

	struct requestMsg req;
	struct responseMsg res;

	/* if given length is not valid, print help message and terminate */
	length = strtol(argv[1], &endptr, 10);
	if (length == LONG_MIN || length == LONG_MAX || length < 0 || length > INT_MAX || *endptr != '\0')
		helpAndExit(argv[0], EXIT_FAILURE);

	/* get server's message queue */
	msgqid = msgget(MSGQ_KEY, S_IRUSR | S_IWUSR); /* client needs to read and write to the queue */
	if (msgqid == -1)
		pexit("msgget");

	pid = getpid();

	req.mtype = SERVER_MSG_TYPE;
	req.pid = pid;
	req.seqLen = length;

	/* sends request to the server */
	if (msgsnd(msgqid, &req, REQ_MSG_LEN, 0) == -1)
		pexit("msgsnd");

	/* awaits a response */
	if (msgrcv(msgqid, &res, RESP_MSG_LEN, pid, 0) == -1) /* message type is the client PID */
		pexit("msgrcv");

	printf("Sequence Number: %d\n", res.seqNum);

	exit(EXIT_SUCCESS);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [seqLen]\n", progname);
	exit(status);
}
