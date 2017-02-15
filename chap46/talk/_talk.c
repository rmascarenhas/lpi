/* _talk.c - A simple implementation of the talk(1) utility (client process.)
 *
 * This is the client program for the this talk implementation. It receives as
 * an argument the username of the user to connect to (start talking with.) The
 * client communicates with the daemon (see _talkd.c) in order to have the
 * connection request carried out. Once the two sides of the communication have
 * agreed to talk, chatting can commence.
 *
 * Usage:
 *
 *   $ ./_talk [username]
 *   username - the username of a currently logged in user.
 *
 * Author: Renato Mascarenhas Costa
 */

#include "common.h"

static void cleanup(void);
static void logout(void);
static void helpAndExit(const char *progname, int status);
static void childHandler(int sig);

static void spawnListener(void);
static void chatLoop(void);

/* make them accessible througout the client code so that the cleanup funciton
 * can delete the message queue on termination */
static int serverId;
static int clientId;
static pid_t childPid = 1;

/* pointer to the recipient of the chat. This is assigned to argv[1] once
 * execution starts */
static char *recipient;

int
main(int argc, char *argv[]) {
	struct requestMsg req;
	struct responseMsg res;
	struct sigaction sa;
	ssize_t msgLen;

	if (argc != 2) {
		helpAndExit(argv[0], EXIT_FAILURE);
	}

	serverId = msgget(SERVER_KEY, S_IWUSR);
	if (serverId == -1)
		pexit("msgget");

	clientId = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR | S_IWGRP);
	if (clientId == -1)
		pexit("msgget");

	if (atexit(cleanup) != 0)
		pexit("atexit");

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = childHandler;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		pexit("sigaction");

	recipient = argv[1];

	/* Request connection to talk with username on argv[1] */
	req.mtype = TALK_MT_REQ_CONNECT;
	req.clientId = clientId;
	strncpy(req.fromUsername, getlogin(), LOGIN_NAME_MAX);
	strncpy(req.toUsername, recipient, LOGIN_NAME_MAX);

	printf("Requesting connection...\n");
	if (msgsnd(serverId, &req, TALK_REQ_MSG_SIZE, 0) == -1)
		pexit("msgsnd");

	msgLen = msgrcv(clientId, &res, TALK_RES_MSG_SIZE, 0, 0);
	if (msgLen == -1)
		pexit("msgrcv");

	switch (res.mtype) {
		case TALK_MT_RES_CONNECT_ACCEPT:
			printf("Connected.\n");
			spawnListener();
			chatLoop();
			break;
		case TALK_MT_RES_CONNECT_FAILURE:
			printf("Error: %s\n", res.data);
			break;
		default:
			fprintf(stderr, "Unexpected response from the server: %ld\n", res.mtype);
			exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

static void
logout() {
	struct requestMsg req;

	req.mtype = TALK_MT_REQ_TALK_CONN_DROP;
	strncpy(req.fromUsername, getlogin(), LOGIN_NAME_MAX);
	strncpy(req.toUsername, recipient, LOGIN_NAME_MAX);

	msgsnd(serverId, &req, TALK_RES_MSG_SIZE, 0);

	/* terminate the child listening for incoming messages */
	if (childPid != 1) {
		if (kill(childPid, SIGTERM) == -1)
			pexit("kill");
	}
}

static void
chatLoop() {
	char *me = getlogin();
	char buf[BUFSIZ];
	char *prompt = ">> ";
	struct requestMsg req;

	printf(prompt);
	while (fgets(buf, BUFSIZ, stdin) != NULL) {
		/* drop newline character if present */
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';

		req.mtype = TALK_MT_REQ_TALK_MSG;
		strncpy(req.fromUsername, me, LOGIN_NAME_MAX);
		strncpy(req.toUsername, recipient, LOGIN_NAME_MAX);

		strncpy(req.data, buf, DATA_SIZE);
		req.data[DATA_SIZE - 1] = '\0';

		if (msgsnd(serverId, &req, TALK_REQ_MSG_SIZE, 0) == -1)
			pexit("msgsnd");

		printf("[%s] %s\n", me, req.data);
		printf(prompt);
	}

	/* user wants to disconnect - logout from the server */
	logout();
}

static void
spawnListener() {
	ssize_t msgLen;
	struct requestMsg req;

	switch (childPid = fork()) {
		case -1:
			pexit("fork");

		case 0:
			/* child: continue execution */
			break;

		default:
			/* parent: return to main */
			return;
	}

	/* child: listen the client message queue for incoming messages from the
	 * other end of the conversation */
	for (;;) {
		msgLen = msgrcv(clientId, &req, TALK_REQ_MSG_SIZE, 0, 0);

		/* if reading from the queue fails, terminate with a failure status, to be picked
		 * by the the parent process */
		if (msgLen == -1)
			_exit(EXIT_FAILURE);

		switch (req.mtype) {
			case TALK_MT_REQ_TALK_MSG:
				printf("\n[%s] %s\n", recipient, req.data);
				break;
			case TALK_MT_REQ_TALK_CONN_DROP:
				/* in case the connection was dropped from the other side of the connection,
				 * terminate, since there is no one listening */
				_exit(EXIT_SUCCESS);
			default:
				fprintf(stderr, "Error: Received unknown message type from the server: %ld.\n", req.mtype);
				_exit(EXIT_FAILURE);
		}
	}
}

static void
childHandler(__attribute__((unused)) int sig) {
	int status, exitStatus;

	if (waitpid(childPid, &status, WNOHANG) == -1)
		pexit("waitpid");

	/* the child process, reading messages from the queue, terminated.
	 * Print a feedback message to the user depending on the termination
	 * status, and exit */
	if (WIFEXITED(status)) {
		exitStatus = WEXITSTATUS(status);

		if (exitStatus == EXIT_SUCCESS) {
			printf("\nConnection dropped by remote user. Terminating.\n");
			exit(EXIT_SUCCESS);
		}

		printf("\nError processing incoming message. Terminating.\n");
		exit(EXIT_FAILURE);
	}
}

static void
cleanup() {
	if (msgctl(clientId, IPC_RMID, NULL) == -1)
		pexit("msgctl");
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [username]\n", progname);
	exit(status);
}
