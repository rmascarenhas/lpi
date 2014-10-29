/* console.c - a console for interacting with the thread-safe binary tree.
 *
 * The binary tree implementation allows the user to add, delete and lookup
 * values. This console allows the user to interact with the data structure
 * in a REPL fashion. Available commands are:
 *
 * help - print info on available commands.
 * add [key, val] - adds a node to the tree.
 * delete [key] - tries to delete a node with the given key.
 * lookup [key] - retrieves the data associated with the given key.
 * print - prints all keys available in sorted order (needs debug support).
 * visual [file] - saves a visual representation of the tree to the given file (needs debug support).
 * quit - finish session.
 *
 * The aim of this program is not to test the tsbintree library. See the `test/`
 * directory for files that actually test its behavior in multi-threaded scenarios.
 *
 * Note that every data inserted on the tree using this program must be a char *.
 * This is not imposed by the library, but by this console program. This is to make
 * it debugging and printing easier.
 *
 * Usage
 *
 *    $ ./console
 *
 * Author: Renato Mascarenhas Costa.
 *
 * Obs: this has a series of memory management problems (e.g., deleted keys do not
 * are not `free`d. This is so to make it easier for the console, since its
 * purpose was not completeness, but just being a tool for simple tests.
 */

#define _BSD_SOURCE

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tsbintree.h"

#define TSBT_DOT_SCRIPT_SIZE (1024 * 1024)

#ifndef TSBT_PROMPT
#  define TSBT_PROMPT ("tsbintree> ")
#endif

#ifndef DOT_PROG
#  define DOT_PROG ("dot")
#endif

#define CmdError(...) fprintf(stderr, "=> ERROR: " __VA_ARGS__)

static void fatal(const char *fCall);

static void printHelp(void);
static void doAdd(tsbintree *bt, char *key, char *value);
static void doDelete(tsbintree *t, char *key);
static void doLookup(tsbintree *bt, char *key);

#ifdef TSBT_DEBUG
static void doPrint(tsbintree *bt);
static void doVisual(tsbintree *bt, char *filename);
#endif

int
main() {
	char buf[BUFSIZ];
	char *command, *arg1, *arg2;
	ssize_t numRead;

	/* disable output buffering */
	setbuf(stdout, NULL);

	/* create the data structure we are going to be using */
	tsbintree bt;
	tsbintree_init(&bt);

	for (;;) {
		printf(TSBT_PROMPT);
		if ((numRead = read(STDIN_FILENO, buf, BUFSIZ)) == BUFSIZ) {
			printf("=> ERROR: input string too long\n\n");
		} else if (numRead > 1) {
			buf[numRead - 1] = '\0'; /* strip trailing newline character */
			command = strtok(buf, " ");

			if (!strncmp(command, "help", BUFSIZ)) {
				printHelp();
			} else if (!strncmp(command, "add", BUFSIZ)) {
				arg1 = strtok(NULL, " "); /* key */
				arg2 = strtok(NULL, " "); /* value */
				doAdd(&bt, arg1, arg2);
			} else if (!strncmp(command, "delete", BUFSIZ)) {
				arg1 = strtok(NULL, " "); /* key to be deleted */
				doDelete(&bt, arg1);
			} else if (!strncmp(command, "lookup", BUFSIZ)) {
				arg1 = strtok(NULL, " "); /* key to be searched */
				doLookup(&bt, arg1);
			}
#ifdef TSBT_DEBUG
			else if (!strncmp(command, "print", BUFSIZ)) {
				doPrint(&bt);
			} else if (!strncmp(command, "visual", BUFSIZ)) {
				arg1 = strtok(NULL, " "); /* image filename */
				doVisual(&bt, arg1);
			}
#endif
			else if (!strncmp(command, "exit", BUFSIZ) || !strncmp(command, "quit", BUFSIZ)) {
				if (tsbintree_destroy(&bt) == -1) {
					CmdError("tsbintree_destroy: %s\n\n", strerror(errno));
				}

				printf("Bye.\n");
				exit(EXIT_SUCCESS);
			} else {
				CmdError("%s: no such command\n\n", command);
			}
		}
	}
}

static void
printHelp() {
	printf("Available commands are:\n");
	printf("%20s%70s\n", "help", "prints info on available commands");
	printf("%20s%70s\n", "add <key> <val>", "adds a node to the tree");
	printf("%20s%70s\n", "delete <key>", "tries to delete a node with the given key");
	printf("%20s%70s\n", "lookup <key>", "retrieves the data associated with the given key");
#ifdef TSBT_DEBUG
	printf("%20s%70s\n", "print", "prints all keys available in sorted order");
	printf("%20s%70s\n", "visual <file>", "saves a visual representation of the tree to the given file");
#endif
	printf("%20s%70s\n\n", "quit | exit", "finishes session");
}

static void
doAdd(tsbintree *bt, char *key, char *value) {
	if (key == NULL || value == NULL) {
		CmdError("Syntax: add <key> <value>\n\n");
		return;
	}

	/* make copies of the given values */
	size_t keylen, valuelen;
	keylen = strlen(key) + 1;
	valuelen = strlen(value) + 1;

	char *newkey = malloc(keylen);
	if (newkey == NULL) fatal("malloc");
	char *newvalue = malloc(valuelen);
	if (newvalue == NULL) fatal("malloc");

	strncpy(newkey, key, keylen);
	strncpy(newvalue, value, valuelen);

	if (tsbintree_add(bt, newkey, newvalue) == -1) {
		CmdError("tsbintree_add: %s\n\n", strerror(errno));
	} else {
		printf("=> Added %s=%s\n\n", key, value);
	}
}

static void
doDelete(tsbintree *bt, char *key) {
	if (key == NULL) {
		CmdError("Syntax: delete <key>\n");
		return;
	}

	if (tsbintree_delete(bt, key) == -1) {
		if (errno == ENOKEY) {
			CmdError("No such key: %s\n\n", key);
		} else {
			CmdError("tsbintree_delete: %s\n\n", strerror(errno));
		}
	} else {
		printf("=> Deleted key %s\n\n", key);
	}
}

static void
doLookup(tsbintree *bt, char *key) {
	void *val = NULL;

	if (key == NULL) {
		CmdError("Syntax: lookup <key>\n\n");
		return;
	}

	if (tsbintree_lookup(bt, key, &val) == -1) {
		CmdError("tsbintree_lookup: %s\n\n", strerror(errno));
	} else {
		if (val == NULL) {
			printf("=> No data associated with \"%s\" key.\n\n", key);
		} else {
			printf("=> %s=%s\n\n", key, (char *) val);
		}
	}
}


#ifdef TSBT_DEBUG
static void
doPrint(tsbintree *bt) {
	int nodes;

	if ((nodes = tsbintree_print(bt)) == 0) {
		printf("=> Binary tree is empty\n");
	}

	printf("(%d elements)\n", nodes);
}

static void
doVisual(tsbintree *bt, char *filename) {
	if (filename == NULL) {
		CmdError("Syntax: visual <file>\n\n");
		return;
	}

	char *script, command[BUFSIZ];
	char tmptemplate[] = "/tmp/_tsbintree_console_XXXXXX";
	int status, tmpfd;
	errno = 0;

	script = malloc(TSBT_DOT_SCRIPT_SIZE);
	if (script == NULL) {
		CmdError("malloc: %s\n\n", strerror(errno));
		return;
	}

	if (tsbintree_to_dot(bt, script, TSBT_DOT_SCRIPT_SIZE) == -1) {
		if (errno == 0) {
			printf("=> Could not generate tree. Maybe it is too big?\n\n");
		} else {
			CmdError("tsbintree_to_dot: %s\n\n", strerror(errno));
		}

		return;
	}

	/* create a temporary file to hold the script content */
	tmpfd = mkstemp(tmptemplate);
	if (tmpfd == -1)
		CmdError("mktemp: %s\n\n", strerror(errno));

	if ((status = write(tmpfd, script, strlen(script))) == -1)
		CmdError("write: %s\n\n", strerror(errno));

	snprintf(command, BUFSIZ, "%s -Tpng -o %s %s", DOT_PROG, filename, tmptemplate);
	status = system(command);

	if (status == -1)
		CmdError("system: %s\n\n", strerror(errno));

	if (WEXITSTATUS(status) == 0) {
		printf("=> Graph representation saved at %s\n\n", filename);
	} else {
		printf("=> Error generating graph. Do you have %s installed?\n\n", DOT_PROG);
	}

	unlink(tmptemplate);
	free(script);
}
#endif

static void
fatal(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
