/* parser.h - parser for the commands allowed by the `nv` tool.
*
* `nv` is a simple utility that provides access to a name-value data-structure
* using a System V shared memory segment. As such, multiple runs of `nv` commands
* can read and write to this shared memory segment concurrently.
*
* See implementation and README.md files for more information on implementation
* and usage.
*
* Author: Renato Mascarenhas Costa
*/

#ifndef PARSER_H
#define PARSER_H

/* get XSI-compliant version of `strerror_r(3)` */
#define _POSIX_C_SOURCE (200112L)

#define MAX_CMD_LEN (64)
#define MAX_ARGS (8)
#define MAX_ARG_LEN (1024)

#define BUF_SIZE (2048)

#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* command definitions. These are arbitrary codes used to represent each possible
 * operation in a script */
#define CMD_SET         (0)
#define CMD_SET_IF_NONE (1)
#define CMD_ASSIGN      (2)
#define CMD_GET         (3)
#define CMD_DELETE      (4)
#define CMD_PRINT       (5)

struct compilationError {
	int lineno;             /* line number containing the error */
	char cmd[MAX_CMD_LEN];  /* the command that caused the error */
	char message[BUF_SIZE]; /* a descriptive error message */
};

struct command {
	int code;             /* code of the command to be run. See definitions */
	char *args[MAX_ARGS]; /* an array of arguments */
	int nargs;            /* the number of arguments (i.e., length of `args` array) */
};

struct program {
	int fd;              /* file descriptor pointing to the resource containing the script */
	struct command *ops; /* list of commands in the program */
	int nops;            /* number of operations (i.e., length of `ops` array) */
};

/* creates a new program data structure. Receives as argument a file descriptor
 * which should be pointing to a resource containing the actual content of the
 * script to be run.
 *
 * Returns -1 on error. */
int initScript(int fd, struct program *ds);

/* Reads the content of a program (which should have been created via a `initScript`
 * function call), and populates its internal data structures. Any compilation errors,
 * if found, are returned in the `compilationError` struct.
 *
 * Returns -1 when there is an internal error (e.g., the system is out of memory)
 * or when there is a compilation error; in the latter scenario, the `compilationError`
 * struct is filled accordingly */
int compileScript(struct program *ds, struct compilationError *error);

/* destroys associated data structures of a `program` struct. */
void destroyScript(struct program *ds);

#endif /* PARSER_H */
