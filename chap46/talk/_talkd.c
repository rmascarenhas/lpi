/* _talkd.c - A simple implementation of the talk(1) utility (daemon process.)
 *
 * talk(1) is a utility that allows users in the same network to send messages
 * and talk to each other. The implementation being distributed on most Unix
 * systems also allows communication to happen among users on different
 * host machines.
 *
 * This program provides a similar, simpler functionality using System V Message
 * Queues. This means that this program can only allow communication among users
 * on the same machine, and not over the network.
 *
 * See the client program (_talk.c) on instructions on how to actually communicate
 * using this software.
 *
 * How it works:
 *
 * This server is inteded to be run as a daemon. As such, connection requests, as
 * well as messages being exchanged among users go through this server. Clients
 * know only how to send messages to the daemon requesting things to be done on
 * their behalf.
 *
 * This daemon serves requests coming from clients using a fork mechanism: this means
 * that each incoming request will cause a new process to be created to serve it. A
 * maximum of MAX_CHILDREN child processes must exist at one given time - if the limit
 * is reached, incoming requests will be rejected.
 *
 * Connections are managed using a series of files in a temporary directory used by
 * this program (located at TALK_CONN_DIR.) If user u1 wants to talk to user u2,
 * the server creates the a file named `u1:u2` in the temporary directory. The content
 * of this file is the message queue ID where `u1`'s client is waiting for messages.
 * If `u2` accepts the invitation to talk, and issue a command to talk to `u1`, another
 * file, `u2:u1` is created containing the message queue ID of `u2`. Since the counterpart
 * connection was previously established, both clients are sent a connection accept message,
 * and can start talking.
 *
 * When a client is disconnects, both files `u1:u2` and `u2:u1` are removed. In order
 * to communicate again, a new connection request needs to be sent.
 *
 * Usage:
 *
 *   $ ./_talkd
 *
 * Author: Renato Mascarenhas Costa
 */

/* get definition of `strsignal` */
#define _GNU_SOURCE
#define _XOPEN_SOURCE (700)

#include "common.h"
#include <utmp.h>
#include <utmpx.h>
#include <ftw.h>
#include <syslog.h>
#include <stdbool.h>

#define MAX_CHILDREN       (128)                    /* do not fork more than MAX_CHILDREN processes */
#define CONN_FILE_TEMPLATE (TALK_CONN_DIR "/%s:%s") /* template for full path of connection files */

#define T_VERIFY (1) /* if the connection already exists, do not error out */

#define TALK_PATH_UTMP ("/run/utmp") /* this varies according to Linux distribution. Edit accordingly */

#define BUFSIZE (1024)
#define PROGNAME ("_talk")

/* if system does not define the maximum number of open file descriptors,
 * make a guess */
#define MAX_FD_GUESS (8192)

static void init(void);
static void cleanup(void);
static void cleanupHandler(int sig);
static void childHandler(int sig);
static void serveRequest(const struct requestMsg *req);

static key_t serverId;

int
main() {
	struct requestMsg req;
	pid_t pid;
	ssize_t msgLen;
	struct sigaction sa;

	/* SIGCHLD to reap terminated children (serving requests) */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = childHandler;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		pexit("sigaction");

	/* cleanup when a deadly signal is received */
	/* SIGINT */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = cleanupHandler;
	if (sigaction(SIGINT, &sa, NULL) == -1)
		pexit("sigaction");

	/* SIGTERM */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = cleanupHandler;
	if (sigaction(SIGTERM, &sa, NULL) == -1)
		pexit("sigaction");
	if (atexit(cleanup) == -1)
		pexit("atexit");

	init();

	/* loop reading requests, handle each in a separate child process */
	for (;;) {
		msgLen = msgrcv(serverId, &req, TALK_REQ_MSG_SIZE, 0, 0);
		if (msgLen == -1) {
			if (errno == EINTR)
				continue; /* Interrupted by the SIGCHLD handler - continue work */

			break; /* other error - break out of the loop */
		}

		pid = fork();
		if (pid == -1) {
			perror("fork");
			break;
		}

		if (pid == 0) {
			serveRequest(&req);  /* child process - serve the request */
			_exit(EXIT_SUCCESS); /* reached this point - request was served successfully */
		}

		/* parent loops to wait for the next message */
	}

	/* error on `msgrcv` or `fork` - remove the message queue and terminate */
	if (msgctl(serverId, IPC_RMID, NULL) == -1)
		pexit("msgctl");

	exit(EXIT_SUCCESS);
}

/* transforms the running process into a daemon process. Inspired by listing on
 * section 37.2 of The Linux Programming Interface, by Michael Kerrisk */
static void
becomeDaemon() {
	int fd, maxfd;

	/* 1. become a background process */
	switch (fork()) {
		case -1: pexit("fork");
		case 0:  break;
		default: _exit(EXIT_SUCCESS);
	}

	/* 2. create a new session */
	if (setsid() == -1) {
		syslog(LOG_EMERG, "Failed to become a session leader, terminating.");
		pexit("setsid");
	}

	/* 3. do not be the session leader - fork a new child again */
	switch (fork()) {
		case -1: pexit("fork");
		case 0:  break;
		default: _exit(EXIT_SUCCESS);
	}

	/* 4. Clear file mode creation mask */
	umask(0);

	/* 5. Change to the root directory */
	chdir("/");

	/* 6. Close all open files */
	maxfd = sysconf(_SC_OPEN_MAX);
	if (maxfd == -1) /* if not available, guess */
		maxfd = MAX_FD_GUESS;

	for (fd = 0; fd < maxfd; ++fd)
		close(fd);

	/* 7. Reopen standard file descriptors to /dev/null */
	close(STDIN_FILENO);
	fd = open("/dev/null", O_RDWR);

	if (fd != STDIN_FILENO) {
		syslog(LOG_EMERG, "Failure to reopen standard input on /dev/null");
		pexit("stdin");
	}

	if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO) {
		syslog(LOG_EMERG, "Failure to reopen standard output on /dev/null");
		pexit("stdout");
	}

	if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO) {
		syslog(LOG_EMERG, "Failure to reopen standard error on /dev/null");
		pexit("stderr");
	}
}

static void
init() {
	int fd;
	char id[MAX_SV_QUEUE_ID_LEN];

	if (mkdir(TALK_CONN_DIR, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) == -1) {
		/* temporary directory already exists - probably because there is another
		 * instance of the daemon already running, or a previous run was killed
		 * with SIGKILL */
		if (errno == EEXIST) {
			fprintf(stderr, "The directory %s already exists. Another instance still running?\n", TALK_CONN_DIR);
			exit(EXIT_FAILURE);
		}

		/* other error happened - abort */
		pexit("mkdir");
	}

	fd = open(SERVER_QID_PATH, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IRGRP);
	if (fd == -1) {
		/* server queue ID file already exists - same situation as temporary directory
		 * check above */
		if (errno == EEXIST) {
			fprintf(stderr, "The file %s already exists. Another instance still running?\n", SERVER_QID_PATH);
			exit(EXIT_FAILURE);
		}

		/* another error happened */
		pexit("open");
	}

    serverId = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IWGRP);
    if (serverId == -1)
		pexit("msgget");

	/* write server queue ID to the well-known file for clients to read */
	snprintf(id, MAX_SV_QUEUE_ID_LEN, "%d", serverId);
	if (write(fd, id, strlen(id)) == -1)
		pexit("write");

	if (close(fd) == -1)
		pexit("close");

	/* configure syslog */
	openlog(PROGNAME, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

	/* move to the background */
	becomeDaemon();
}

static int
removalFn(const char *fpath, __attribute__((unused)) const struct stat *sb, int typeflag) {
	if (typeflag == FTW_F) {
		if (unlink(fpath) == -1)
			return -1;
	}

	return 0;
}

static void
cleanup() {
	/* remove the temporary directory where connection files are kept */
	ftw(TALK_CONN_DIR, removalFn, 2);
	if (rmdir(TALK_CONN_DIR) == -1)
		pexit("rmdir");

	/* remove the message queue */
	if (msgctl(serverId, IPC_RMID, NULL) == -1)
		pexit("msgctl");

	/* close syslog */
	closelog();
}

static void
cleanupHandler(int sig) {
	/* let parent process know that the child was killed via a signal, as opposed
	 * to a regular termination */
	syslog(LOG_INFO, "Received %s, terminating", strsignal(sig));
	cleanup();

	signal(sig, SIG_DFL);
	raise(sig);
}

static void
childHandler(__attribute__((unused)) int sig) {
	int savedErrno;

	savedErrno = errno;
	while (waitpid(-1, NULL, WNOHANG) > 0)
		continue; /* wait for every terminated children */

	/* waipid() call above might have changed `errno` */
	errno = savedErrno;
}

static void
connReply(int msgqid, struct responseMsg *res) {
	/* msgsnd errors are not diagnosed since the error cannot be sent back to the client */
	msgsnd(msgqid, res, strlen(res->data) + 1, 0);
}

static void
connFailure(const struct requestMsg *req, const char *reason) {
	struct responseMsg res;

	res.mtype = TALK_MT_RES_CONNECT_FAILURE;
	memcpy(res.data, reason, strlen(reason));

	/* make sure the string is null terminated */
	res.data[strlen(reason)] = '\0';
	connReply(req->clientId, &res);
}

static int
connectionFile(const char *from, const char *to, char *buf, int len) {
	int numWritten;

	numWritten = snprintf(buf, len, CONN_FILE_TEMPLATE, from, to);

	/* error, or file name was truncated */
	if (numWritten < 0 || numWritten == len)
		return -1;

	return 0;
}

static int
writeConnFile(const char *from, const char *to, int queueId) {
	char path[PATH_MAX];
	char id[MAX_SV_QUEUE_ID_LEN];
	int fd;

	if (connectionFile(from, to, path, PATH_MAX) == -1)
		return -1;

	fd = open(path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return -1;

	snprintf(id, MAX_SV_QUEUE_ID_LEN, "%d", queueId);
	write(fd, id, strlen(id));

	close(fd);

	return 0;
}

static int
removeConnFile(const char *from, const char *to) {
	char path[PATH_MAX];

	if (connectionFile(from, to, path, PATH_MAX) == -1)
		return -1;

	if (unlink(path) == -1)
		return -1;

	return 0;
}

static int
readConnFile(const char *from, const char *to, int *dst) {
	int fd;
	char path[PATH_MAX];
	char buf[MAX_SV_QUEUE_ID_LEN];

	if (connectionFile(from, to, path, PATH_MAX) == -1)
		return -1;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		if (errno == ENOENT) {
			*dst = 0;
			return 0;
		}

		return -1;
	}

	if (read(fd, buf, MAX_SV_QUEUE_ID_LEN) == -1)
		return -1;

	if (close(fd) == -1)
		return -1;

	close(fd);

	*dst = (int) strtol(buf, NULL, 10);
	return 0;
}

static int
requestConnection(const char *from, const char *to) {
	struct utmpx *ut;
	char tty[UT_LINESIZE + 1],
		 ttyPath[PATH_MAX],
		 message[BUFSIZE];
	bool loggedIn = false;
	int fd;

	/* 1. find TTY where user is logged in (if any) */
	utmpname(TALK_PATH_UTMP);

	errno = 0;
	setutxent();
	if (errno != 0) {
		syslog(LOG_WARNING, "Could not open utpm file");
		return 0;
	}

	while ((ut = getutxent()) != NULL) {
		/* find a login entry for the recipient username */
		if ((ut->ut_type == INIT_PROCESS || ut->ut_type == LOGIN_PROCESS || ut->ut_type == USER_PROCESS) && !strncmp(ut->ut_user, to, UT_NAMESIZE)) {
			memcpy(tty, ut->ut_line, UT_LINESIZE);
			tty[strlen(ut->ut_line) + 1] = '\0'; /* make sure the TTY name is properly terminated */
			loggedIn = true;
		}
	}

	if (!loggedIn)
		return -1;

	snprintf(ttyPath, PATH_MAX, "/dev/%s", tty);

	fd = open(ttyPath, O_WRONLY | S_IWUSR);
	if (fd == -1) {
		syslog(LOG_WARNING, "Failed to open TTY device: %s", ttyPath);
		return 0;
	}

	/* error handling in the functions above supressed since they cannot be
	 * indicated back to the _talk caller */
	snprintf(message, BUFSIZE, "Message from %s\n", from);
	write(fd, message, strlen(message));
	snprintf(message, BUFSIZE, "%s: connection requested by %s\n", PROGNAME, from);
	write(fd, message, strlen(message));
	snprintf(message, BUFSIZE, "%s: respond with: %s %s\n", PROGNAME, PROGNAME, from);
	write(fd, message, strlen(message));

	fsync(fd);
	close(fd);

	return 0;
}

static int
connectionAccepted(int msgqid) {
	struct requestMsg res;

	res.mtype = TALK_MT_RES_CONNECT_ACCEPT;
	return msgsnd(msgqid, &res, TALK_RES_MSG_SIZE, 0);
}

static void
connect(const struct requestMsg *req) {
	int fromQ, toQ;

	if (readConnFile(req->fromUsername, req->toUsername, &fromQ) == -1) {
		/* error reading the file, request cannot be processed */
		syslog(LOG_ERR, "Could not read connection file (%s -> %s)", req->fromUsername, req->toUsername);
		connFailure(req, "Connection Failure");
		return;
	}

	if (fromQ != 0) {
		/* connection already in place between the two users: no need to reconnect */
		connFailure(req, "Already connected");
		return;
	}

	if (readConnFile(req->toUsername, req->fromUsername, &toQ) == -1) {
		syslog(LOG_ERR, "Could not read connection file (%s -> %s)", req->toUsername, req->fromUsername);
		connFailure(req, "Connection Failure");
		return;
	}

	if (toQ == 0) {
		/* opposite connection does not exist yet - send a request to the recipient's TTY */
		if (requestConnection(req->fromUsername, req->toUsername) == -1) {
			connFailure(req, "User not logged in");
			return;
		}
	} else {
		/* opposite connection already exists - confirm connection on both ends */
		if (connectionAccepted(toQ) == -1 || connectionAccepted(req->clientId) == -1) {
			syslog(LOG_WARNING, "Could not send connection acceptance");
			connFailure(req, "Connection Failure");
			return;
		}
	}

	if (writeConnFile(req->fromUsername, req->toUsername, req->clientId) == -1) {
		syslog(LOG_ERR, "Could not write connection file (%s -> %s)", req->fromUsername, req->toUsername);
		connFailure(req, "Connection Failure");
		return;
	}
}

static void
sendMsg(const struct requestMsg *req) {
	int toQ; /* queue ID of the recipient */
	struct requestMsg fwdReq;

	/* client does not listen to errors from this message. If there is an error,
	 * terminate the processing early */
	if (readConnFile(req->toUsername, req->fromUsername, &toQ) == -1)
		return;

	/* forward the message back to the recipient's message queue */
	fwdReq.mtype = req->mtype;
	memcpy(fwdReq.data, req->data, DATA_SIZE);

	msgsnd(toQ, &fwdReq, TALK_REQ_MSG_SIZE, 0);
}

static void
disconnect(const struct requestMsg *req) {
	/* disconnection - remove both connection files (from->to and to->from) and
	 * send a connection drop message to the recipient so that its client can
	 * inform the target user accordingly */
	int toQ;
	struct requestMsg dropReq;

	if (readConnFile(req->toUsername, req->fromUsername, &toQ) == -1)
		return;

	/* since `fromUsername` is disconnecting, the from -> to connection must exist */
	if (removeConnFile(req->fromUsername, req->toUsername) == -1)
		return;

	/* the opposite connection, to -> from, might not exist (i.e., if the requester
	 * timeout waiting for a reply from the recipient, for example) */
	removeConnFile(req->toUsername, req->fromUsername);

	dropReq.mtype = TALK_MT_REQ_TALK_CONN_DROP;
	msgsnd(toQ, &dropReq, TALK_REQ_MSG_SIZE, 0);
}

static void
serveRequest(const struct requestMsg *req) {
	switch (req->mtype) {
		case TALK_MT_REQ_CONNECT:
			connect(req);
			break;
		case TALK_MT_REQ_TALK_MSG:
			sendMsg(req);
			break;
		case TALK_MT_REQ_TALK_CONN_DROP:
			disconnect(req);
			break;
	}
}
