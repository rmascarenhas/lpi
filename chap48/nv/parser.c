/* parser.c - parser for the commands allowed by the `nv` tool.
*
* This implements the functions defined on the `parser.h` file. It provides an API
* to allow callers to read from script files, identify compilation errors and build
* data structures of the list of commands contained in a script.
*
* Author: Renato Mascarenhas Costa
*/

#include "parser.h"

int
initScript(int fd, struct program *ds) {
	int nfd;

	/* duplicate the given file descriptor for later use - this allows the caller to
	 * manipulate the original file descriptor at will, even closing it, without
	 * affecting later calls to parser functions */
	nfd = dup(fd);
	if (nfd == -1)
		return -1;

	ds->fd = nfd;

	return 0;
}

static void
cleanupLine(char *s) {
	int i, slashIdx;
	bool commentFound;

	/* remove trailing newline, if any */
	if (s[strlen(s) - 1] == '\n')
		s[strlen(s) - 1] = '\0';

	/* remove leading spaces, if any */
	while (isspace(*s))
		s++;

	/* simple inline comment implementation: if two consecutive slashes (`/`) are
	 * found, everything after is removed and not considered on execution.
	 *
	 * Example
	 *
	 * 	print Hello world       // This is a simple statement
	 *
	 * becomes
	 *
	 * 	print Hello world      
	 *
	 * The trailing spaces will not be removed on this step (but *will* later
	 * in this function.)
	 */
	slashIdx = -1;
	commentFound = false;
	for (i = 0; i < (int) strlen(s) && !commentFound; i++) {
		if (s[i] == '/') {
			/* if this is not the first slash found and they are one character apart,
			 * an inline comment is found */
			if (slashIdx != -1 && i - slashIdx == 1) {
				commentFound = true;
				break;
			}

			slashIdx = i;
		}
	}

	/* if there is an inline comment, remove it */
	if (commentFound)
		s[slashIdx] = '\0';

	/* remove any trailing spaces remaining */
	i = strlen(s) - 1;
	while (i >= 0 && isspace(s[i]))
		i--;

	s[i + 1] = '\0';
}

/* when a compilation fails not due to the content of the script, but due to the
 * system its being run (e.g., the system is short of memory when needed for compilation.)
 *
 * The `cmd` field is an empty string, and the `message` field is the system's string
 * representation of the current `errno` */
static void
internalError(struct compilationError *error, int lineno) {
	error->cmd[0] = '\0';
	error->lineno = lineno;
	strerror_r(errno, error->message, BUF_SIZE);
}

static int
verifyCommand(struct command *cmd, const char *cmdName, int lineno, struct compilationError *error) {
	switch (cmd->code) {
		case CMD_SET:
		case CMD_SET_IF_NONE:
			if (cmd->nargs != 2) {
				error->lineno = lineno;
				strncpy(error->cmd, cmdName, MAX_CMD_LEN);
				snprintf(error->message, BUF_SIZE, "Expected 2 arguments, got %d", cmd->nargs);
				return -1;
			}
			break;

		case CMD_ASSIGN:
			if (cmd->nargs < 2) {
				error->lineno = lineno;
				strncpy(error->cmd, cmdName, MAX_CMD_LEN);
				snprintf(error->message, BUF_SIZE, "Expected at least 2 arguments, got %d", cmd->nargs);
				return -1;
			}
			break;

		case CMD_GET:
		case CMD_DELETE:
			if (cmd->nargs != 1) {
				error->lineno = lineno;
				strncpy(error->cmd, cmdName, MAX_CMD_LEN);
				snprintf(error->message, BUF_SIZE, "Expected 1 argument, got %d", cmd->nargs);
				return -1;
			}
			break;

		case CMD_PRINT:
			if (cmd->nargs < 1) {
				error->lineno = lineno;
				strncpy(error->cmd, cmdName, MAX_CMD_LEN);
				snprintf(error->message, BUF_SIZE, "At least one argument is required");
				return -1;
			}
			break;
	}

	return 0;
}

int
compileScript(struct program *ds, struct compilationError *error) {
	int ncommands;
	char buf[BUF_SIZE], *saveptr, *token;
	const char *currCmd;
	int ntokens, nargs, nops, lineno;
	struct command *cmd;
	FILE *stream;
	void *p;

	/* start with a default buffer of 64 commands. This is expanded as necessary,
	 * if the script being parsed is more than 1024 commands long. */
	ncommands = 1024;
	ds->ops = malloc(ncommands * sizeof(struct command));
	if (ds->ops == NULL) {
		internalError(error, 0);
		return -1;
	}

	stream = fdopen(ds->fd, "r");
	if (stream == NULL) {
		internalError(error, 0);
		return -1;
	}

	nops = 0;
	lineno = 0;
	while (fgets(buf, BUF_SIZE, stream) != NULL) {
		lineno++;
		cmd = &ds->ops[nops];
		cleanupLine(buf);

		/* blank line or comment */
		if (strlen(buf) == 0)
			continue;

		/* parse line to find tokens, which are separated by spaces or tabs */
		ntokens = 0;
		token = strtok_r(buf, " \t", &saveptr);
		while (token != NULL) {
			ntokens++;

			/* first token: this is the command to be run */
			if (ntokens == 1) {
				if (strcmp(token, "set") == 0) {
					currCmd = "set";
					cmd->code = CMD_SET;
				} else if (strcmp(token, "setifnone") == 0) {
					currCmd = "setifnone";
					cmd->code = CMD_SET_IF_NONE;
				} else if (strcmp(token, "assign") == 0) {
					currCmd = "assign";
					cmd->code = CMD_ASSIGN;
				} else if (strcmp(token, "get") == 0) {
					currCmd = "get";
					cmd->code = CMD_GET;
				} else if (strcmp(token, "delete") == 0) {
					currCmd = "delete";
					cmd->code = CMD_DELETE;
				} else if (strcmp(token, "print") == 0) {
					currCmd = "print";
					cmd->code = CMD_PRINT;
				} else {
					/* command was not recognized */
					strncpy(error->cmd, token, MAX_CMD_LEN);
					strncpy(error->message, "invalid command", BUF_SIZE);
					return -1;
				}
			} else {
				nargs = ntokens - 1;
				cmd->args[nargs - 1] = malloc(MAX_ARG_LEN);
				if (cmd->args[nargs - 1] == NULL)
					internalError(error, lineno);

				strncpy(cmd->args[nargs - 1], token, MAX_ARG_LEN);
				cmd->nargs = nargs;
			}


			token = strtok_r(NULL, " \t", &saveptr);
		}

		if (verifyCommand(cmd, currCmd, lineno, error) == -1)
			return -1;

		nops++;
		/* increase buffer for command structs if the number of lines in the script
		 * exceeds it */
		if (nops > ncommands) {
			ncommands *= 2;
			p = realloc(ds->ops, (ncommands * sizeof(struct command)));

			if (p == NULL) {
				internalError(error, lineno);
				return -1;
			}

			ds->ops = p;
		}
	}

	/* no longer needed - close the FILE stream */
	fclose(stream);

	ds->nops = nops;
	return nops;
}

void
destroyScript(struct program *ds) {
	int i, j;
	struct command cmd;

	for (i = 0; i < ds->nops; i++) {
		cmd = ds->ops[i];

		for (j = 0; j < cmd.nargs; j++)
			free(cmd.args[j]);
	}

	free(ds->ops);
	close(ds->fd);
}
