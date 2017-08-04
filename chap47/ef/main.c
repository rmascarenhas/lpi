/* main.c - example program to make use of the event flags implementation in this directory
 *
 * This program exercises the event flags implementation available in the ef.h and ef.c
 * files in this directory. It accepts command line parameters that allow event flags to
 * be created, set, cleared, waited for, among others. Run with the '-h' flag for a list
 * of options.
 *
 * Usage
 *
 * 		$ ./main -c set
 * 		237637 # id of a new, set event flag
 *
 * 		$ ./main -g 237637
 * 		set
 *
 * Run with the `-h` flag for a list of options.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ef.h"

#define BUF_SIZE (1024)

static void helpAndExit(const char *progname, int status);
static void pexit(const char *fname);

static void createEventFlag(char *state);
static void setEventFlag(char *id);
static void clearEventFlag(char *id);
static void getEventFlag(char *id);
static void waitForEventFlag(char *id);
static void deleteEventFlag(char *id);

/* defaults for the event flags library */
bool efUseSemUndo = false;
bool efRetryOnEintr = true;

int
main(int argc, char *argv[]) {
	char opt;

	if (argc == 1) {
		helpAndExit(argv[0], EXIT_SUCCESS);
	}

	while ((opt = getopt(argc, argv, "+c:s:x:g:w:d:h")) != -1) {
		switch (opt) {
			case 'c':
				createEventFlag(optarg);
				break;
			case 's':
				setEventFlag(optarg);
				break;
			case 'x':
				clearEventFlag(optarg);
				break;
			case 'g':
				getEventFlag(optarg);
				break;
			case 'w':
				waitForEventFlag(optarg);
				break;
			case 'd':
				deleteEventFlag(optarg);
				break;
			case 'h':
				helpAndExit(argv[0], EXIT_SUCCESS);
				break;
		}
	}

	exit(EXIT_SUCCESS);
}

static void
fatal(const char *message) {
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

static char *
currTime() {
	static char buf[BUF_SIZE];
	time_t t;
	size_t s;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	if (tm == NULL)
		return NULL;

	s = strftime(buf, BUF_SIZE, "%Y-%m-%d %H:%M:%S", tm);
	return (s == 0) ? NULL : buf;
}

static int
parseId(char *id) {
	char *endptr;
	long res;

	res = strtol(id, &endptr, 10);
	if (*endptr != '\0' || res <= 0 || res >= INT_MAX) {
		return -1;
	}

	return res;
}

static void
createEventFlag(char *state) {
	if (strncmp(state, "set", 4) && strncmp(state, "clear", 6))
		fatal("Invalid state requested. Accepted values: 'set' or 'clear'");

	int s = !strncmp(state, "set", 4) ? EF_SET : EF_CLEAR,
		id = efCreate(s);

	if (id == -1)
		pexit("efCreate");

	printf("[%ld][%s] %d\n", (long) getpid(), currTime(), id);
}

static void
setEventFlag(char *id) {
	int semId;

	semId = parseId(id);
	if (semId == -1) {
		fprintf(stderr, "Invalid event flag identifier: %s\n", id);
		return;
	}

	if (efSet(semId) == -1)
		pexit("efSet");
	else
		printf("[%ld][%s] %d: set\n", (long) getpid(), currTime(), semId);
}

static void
clearEventFlag(char *id) {
	int semId;

	semId = parseId(id);
	if (semId == -1) {
		fprintf(stderr, "Invalid event flag identifier: %s\n", id);
		return;
	}

	if (efClear(semId) == -1)
		pexit("efClear");
	else
		printf("[%ld][%s] %d: clear\n", (long) getpid(), currTime(), semId);
}

static void
getEventFlag(char *id) {
	int semId, state;

	semId = parseId(id);
	if (semId == -1) {
		fprintf(stderr, "Invalid event flag identifier: %s\n", id);
		return;
	}

	state = efGet(semId);
	if (state == -1)
		pexit("efGet");

	printf("[%ld][%s] %d: %s\n", (long) getpid(), currTime(), semId, state == EF_SET ? "set" : "clear");
}

static void
waitForEventFlag(char *id) {
	int semId;

	semId = parseId(id);
	if (semId == -1) {
		fprintf(stderr, "Invalid event flag identifier: %s\n", id);
		return;
	}

	printf("[%ld][%s] %d: Waiting for flag to be set\n", (long) getpid(), currTime(), semId);

	if (efWait(semId) == -1)
		pexit("efWait");

	printf("[%ld][%s] %d: Flag is now set\n", (long) getpid(), currTime(), semId);
}

static void
deleteEventFlag(char *id) {
	int semId;

	semId = parseId(id);
	if (semId == -1) {
		fprintf(stderr, "Invalid event flag identifier: %s\n", id);
		return;
	}

	if (efDestroy(semId) == -1)
		pexit("efDestroy");

	printf("[%ld][%s] %d: destroyed\n", (long) getpid(), currTime(), semId);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [-c [state]] [-s [id]] [-x [id]] [-g [id]] [-w [id]] [-d [id]]\n", progname);
	fprintf(stream, "\t%-10s%-50s\n", "-c", "Creates a new event flag");
	fprintf(stream, "\t%-10s%-50s\n", "-s", "Sets an event flag");
	fprintf(stream, "\t%-10s%-50s\n", "-x", "Clears an event flag");
	fprintf(stream, "\t%-10s%-50s\n", "-g", "Gets the current value of an event flag");
	fprintf(stream, "\t%-10s%-50s\n", "-w", "Waits for an event flag to be set");
	fprintf(stream, "\t%-10s%-50s\n", "-d", "Deletes an event flag");

	exit(status);
}

static void
pexit(const char *fname) {
	perror(fname);
	exit(EXIT_FAILURE);
}
