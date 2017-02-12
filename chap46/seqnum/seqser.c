/* seqser.c - A simple sequence server using System V message queues (server).
 *
 * System V message queues is a means of inter process communication. It allows
 * bidirectional message passing, and is not stream based: that is, messages are clearly
 * defined, and it is not possible to read part of a message, or multiple messages
 * at once.
 *
 * This is a simple application that illustrates the use of System V Message Queues.
 * It is composed of two files: seqser.c, the server program (this file) and seqcli.c,
 * the client program.
 *
 * The simplest design was chosen for this program: a single message queue is used for
 * communication between the server and all its clients. This has a number of drawbacks
 * though: a broken or malicious client can make many requests but not read any of them,
 * causing the message queue to fill up and blocking the server and all other clients;
 * and clients must have a way to find out how to read responses for their requests,
 * instead of reading responses for requests coming from other clients: to solve this issue,
 * this program uses the client process ID as the response message type, and clients read
 * only messages of that type.
 *
 * Caveats:
 *
 * - apart from the design issue around having one single message queue for communication,
 *   this server also has little to none error detection and recovery. This is just a simple
 *   experiment with System V Message Queues. For a more robust implementation using the same
 *   IPC mechanism, see the talk(1) that is included in the solutions for this chapter.
 *
 * Based on similar program included in The Linux Programming Interface book.
 *
 * Usage:
 *
 *   $ ./seqser
 *
 * Author: Renato Mascarenhas Costa
 */

#include "common.h"

int
main() {
	struct requestMsg req;
	struct responseMsg res;
	int msgqid;
	ssize_t msgLen;
	int seqNum = 0; /* current sequence number, incremented on every client request */

	msgqid = msgget(MSGQ_KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IWGRP); /* rw--w---- */
	if (msgqid == -1)
		pexit("msgget");

	printf("Server started. Message Queue ID: %d\n", msgqid);

	/* Loop reading client requests, and process them one at a time */
	for (;;) {
		msgLen = msgrcv(msgqid, &req, REQ_MSG_LEN, SERVER_MSG_TYPE, 0); /* read messages of directed to the server */
		if (msgLen == -1)
			pexit("msgrcv");

		res.mtype = req.pid;
		res.seqNum = seqNum;

		if (msgsnd(msgqid, &res, RESP_MSG_LEN, 0) == -1)
			pexit("msgsnd");

		printf(">> Client request completed (pid=%ld seqLen=%d seqNum=%d)\n", (long) req.pid, req.seqLen, seqNum);
		seqNum += req.seqLen;
	}

	exit(EXIT_SUCCESS);
}
