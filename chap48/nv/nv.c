/* nv.c - a name-value tool using System V shared memory.
*
* `nv(1)` is a simple tool to demonstrate the usage of System V shared
* memory as a means of interprocess communication. It is able to parse
* scripts and execute them based on the contents of a shared memory
* segment, possibly previously created and concurrently accessed by other
* `nv(1)` processes.
*
* Usage:
*
* 	Passing a script file
* 		$ ./nv example_script.txt
*
* 	If no script file is passed, standard input is read
* 		$ ./nv
* 		print Hello World!
* 		^D
*
* 	Creating a persistent shared memory segment
* 		$ ./nv -p
* 		12345
*
* 	Using a previously created shared memory segment when running a script
* 		$ ./nv -m 12345 example_script.txt
*
* 	Cleaning up a previously created shared memory segment
* 		$ ./nv -d 12345
*
* By default, `nv` creates a new, temporary shared memory segment to be used
* as memory space for the execution of the given script, which is deleted at
* the end of execution. However, the `-p` parameter instructs `nv` to create
* a shared memory segment and print out its identifier. This identifier can
* later be used with the `-m` parameter - in this case, `nv` will use the
* existing shared memory segment. The combined usage of the `-p` and `-m`
* parameters allow concurrent access of the same shared memory segment by
* multiple `nv` processes.
*
* Author: Renato Mascarenhas Costa
*/

#define _XOPEN_SOURCE /* getopt(3) definition. Mandatory for requiring sys/ipc.h */

#include "parser.h"
#include "ds.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

/* allows compiler to override shared memory segment size - defaults to enough
 * memory to fit 1,000 name/value pairs. Can also be overriden by `-c` parameter */
#ifndef MAX_NV_PAIRS
#	define MAX_NV_PAIRS (1000)
#endif

/* maximum number of variables allowed (created via `assign` command) */
#ifndef NV_VT_SIZE
#	define NV_VT_SIZE (128)
#endif

/* to which variable the contents of a `get` command are saved */
#ifndef NV_GET_VAR
#	define NV_GET_VAR ("_")
#endif

#define SHM_PERMS (S_IRUSR | S_IWUSR)

#define NOP               (0)
#define ACTION_CREATE_SHM (1)

static void fatal(char *msg);
static long stringToLong(char *str);
static void helpAndExit(const char *progname, int status);
static void pexit(const char *fCall);

static int createSharedMem(int cap);
static int initializeSharedMem(int cap);
static void initializeVarTable(void);
static void detachFromMem(void);
static void attachToMem(int id);
static void deleteSharedMem(int id);
static void execute(struct program *program);
static void compilationError(struct compilationError *error);
static void cleanupTempMem(void);

int shmid = -1;   /* shared memory identifier used in this execution */

void *vt = NULL; /* variables table */
void *nv = NULL; /* name/value pairs - always uses a System V shared memory segment */
bool persistent = false; /* whether we are using a pre-created memory segment (`-m` argument) */

int
main(int argc, char *argv[]) {
	int opt, fd, i, cap, action;
	struct program program;
	struct compilationError cerror;

	cap = -1; /* capacity */
	action = NOP;

	opterr = 0;
	while ((opt = getopt(argc, argv, "+pm:c:dh")) != -1) {
		switch (opt) {
			case 'p':
				/* wrong options usage */
				if (action != NOP)
					helpAndExit(argv[0], EXIT_FAILURE);

				action = ACTION_CREATE_SHM;
				break;

			case 'm':
				shmid = (int) stringToLong(optarg);
				persistent = true;
				break;

			case 'c':
				cap = (int) stringToLong(optarg);
				break;

			case 'd':
				for (i = optind; i < argc; i++) {
					shmid = (int) stringToLong(argv[i]);
					deleteSharedMem(shmid);
					printf("%d: deleted\n", shmid);
				}

				exit(EXIT_SUCCESS);

			case 'h':
				helpAndExit(argv[0], EXIT_SUCCESS);
				break;

			default:
				helpAndExit(argv[0], EXIT_FAILURE);
		}
	}

	if (cap == -1)
		cap = MAX_NV_PAIRS;

	/* if we are to just create a shared memory segment for later execution,
	 * create it with the capacity requested (or the default), and terminate */
	if (action == ACTION_CREATE_SHM) {
		shmid = initializeSharedMem(cap);
		printf("%d\n", shmid);

		exit(EXIT_SUCCESS);
	}

	/* if arrived at this point, script execution is to occur */
	if (!persistent) {
		shmid = initializeSharedMem(cap);

		/* registers an `atexit(3)` handler to make sure the shared memory segment
		 * created on this execution (if the `-m` parameter was not passed) is
		 * deleted upon termination. */
		atexit(cleanupTempMem);
	}

	attachToMem(shmid);
	if (persistent) {
		if (dsValidate(nv) == -1)
			pexit("dsValidate");
	}

	initializeVarTable();

	if (optind >= argc) {
		fd = STDIN_FILENO;
	} else {
		fd = open(argv[optind], S_IRUSR);
		if (fd == -1)
			pexit("open");
	}

	if (initScript(fd, &program) == -1)
		pexit("initScript");


	/* original file descriptor no longer needed after program initialization */
	if (close(fd) == -1)
		pexit("close");

	if (compileScript(&program, &cerror) == -1)
		compilationError(&cerror);

	execute(&program);
	destroyScript(&program);

	exit(EXIT_SUCCESS);
}

static void
compilationError(struct compilationError *error) {
	char cmdStr[MAX_CMD_LEN + 2];
	if (strlen(error->cmd) > 0)
		snprintf(cmdStr, MAX_CMD_LEN + 2, " %s:", error->cmd);
	else
		strcpy(cmdStr, "");

	fprintf(stderr, "Compilation error:\n");
	fprintf(stderr, "%d:%s %s\n", error->lineno, cmdStr, error->message);

	exit(EXIT_FAILURE);
}

static void
resolve(char *name, char *buf, int bufsize) {
	char msg[BUF_SIZE];

	if (dsGet(vt, name, buf, bufsize) == -1) {
		if (errno == EINVAL) {
			snprintf(msg, BUF_SIZE, "Runtime error: variable $%s does not exist", name);
			fatal(msg);
		}

		/* if some other error happened, exit immediately */
		pexit("dsGet");
	}
}

static void
set(char *name, char *value) {
	/* write operation: lock reads and writes to the data structure */
	if (dsLock(nv, DS_READ_WRITE) == -1)
		pexit("dsLock");

	if (dsSet(nv, name, value) == -1)
		pexit("dsSet");

	if (dsUnlock(nv, DS_READ_WRITE) == -1)
		pexit("dsUnlock");
}

static void
setifnone(char *name, char *value) {
	char currValue[NVDS_VAL_LEN];

	/* make sure no write operations happen while we check if the name given
	 * has any value associated to it */
	if (dsLock(nv, DS_WRITE) == -1)
		pexit("dsLock");

	if (dsGet(nv, name, currValue, NVDS_VAL_LEN) == -1) {
		if (errno == EINVAL) {
			/* requested `name` does not exist - insert with the given `value` */
			if (dsLock(nv, DS_READ) == -1) /* do not allow read operations while updating */
				pexit("dsLock");

			if (dsSet(nv, name, value) == -1)
				pexit("dsSet");

			if (dsUnlock(nv, DS_READ) == -1)
				pexit("dsUnlock");
		} else {
			/* some other error happened - report */
			pexit("dsGet");
		}
	}

	/* name does not exist, or was already updated - release write lock */
	if (dsUnlock(nv, DS_WRITE) == -1)
		pexit("dsUnlock");
}

static void
assign(char *var, char *value) {
	/* variable table belongs to this process only - no locks needed! */
	if (dsSet(vt, var, value) == -1)
		pexit("dsSet");
}

static void
get(char *name) {
	char buf[NVDS_VAL_LEN];

	/* lock write operations while checking if the name exists */
	if (dsLock(nv, DS_WRITE) == -1)
		pexit("dsLock");

	if (dsGet(nv, name, buf, NVDS_VAL_LEN) == -1) {
		if (errno == EINVAL) {
			/* name requested not found - assign an emptry string to the "$_" variable */
			assign(NV_GET_VAR, "");
		} else {
			/* some other error happened - report */
			pexit("dsGet");
		}
	} else {
		/* value of name requested on 'buf' - copy it to "$_" */
		assign(NV_GET_VAR, buf);
	}

	/* write operations may resume */
	if (dsUnlock(nv, DS_WRITE) == -1)
		pexit("dsUnlock");
}

static void
delete(char *name) {
	/* no writes or reads while deleting data */
	if (dsLock(nv, DS_READ_WRITE) == -1)
		pexit("dsLock");

	if (dsDelete(nv, name) == -1)
		pexit("dsDelete");

	if (dsUnlock(nv, DS_READ_WRITE) == -1)
		pexit("dsDelete");
}

static void
print(char *messages[], int size) {
	int i;
	char *s;

	for (i = 0; i < size; i++) {
		/* represent empty values (i.e., `get` on names that do not exist as a string
		 * "(null)" */
		if (strlen(messages[i]) == 0)
			s = "(null)";
		else
			s = messages[i];

		if (i == 0)
			printf("%s", s);
		else
			printf(" %s", s);
	}

	printf("\n");
}

static void
execute(struct program *program) {
	int i, j, len;
	char buf[MAX_ARG_LEN], str[NVDS_VAL_LEN], arg[NVDS_VAL_LEN], *format, *p;

	printf("Shared memory segment: %p\n\n", nv);

	for (i = 0; i < program->nops; i++) {
		struct command cmd;
		cmd = program->ops[i];

		/* resolve all arguments to expand variables, if any */
		for (j = 0; j < cmd.nargs; j++) {
			if (cmd.args[j][0] == '$') {
				/* if argument is $name, get entry "name" on the var table */
				resolve(&cmd.args[j][1], buf, MAX_ARG_LEN);

				/* copy the result back to the argument list */
				strncpy(cmd.args[j], buf, MAX_ARG_LEN);
			}
		}

		/* in the function calls below, direct access to elements in the args array
		 * is done because the `compileScript` function from parser.c already ensures
		 * each command has the correct number of arguments for execution */
		switch (cmd.code) {
			case CMD_SET:
				set(cmd.args[0], cmd.args[1]);
				break;
			case CMD_SET_IF_NONE:
				setifnone(cmd.args[0], cmd.args[1]);
				break;
			case CMD_ASSIGN:
				p = str;

				for (j = 1; j < cmd.nargs; j++) {
					/* do not prepend a space for the first parameter */
					if (j == 1)
						format = "%s";
					else
						format = " %s";

					snprintf(arg, NVDS_VAL_LEN, format, cmd.args[j]);
					len = strlen(arg);

					memcpy(p, arg, len);

					p += len;
					*p = '\0';
				}

				assign(cmd.args[0], str);
				break;
			case CMD_GET:
				get(cmd.args[0]);
				break;
			case CMD_DELETE:
				delete(cmd.args[0]);
				break;
			case CMD_PRINT:
				print(cmd.args, cmd.nargs);
				break;
		}
	}
}

static void
attachToMem(int id) {
	nv = shmat(id, NULL, 0);

	if (nv == (void *) -1)
		pexit("shmat");
}

static void
detachFromMem() {
	/* detach from the shared memory segment, if previously attached */
	if (nv != NULL) {
		if (shmdt(nv) == -1)
			pexit("shmdt");
	}
}

static void
cleanupTempMem() {
	if (!persistent)
		dsDestroy(nv);

	detachFromMem();
	deleteSharedMem(shmid);

	if (vt != NULL) {
		dsDestroy(vt);
		free(vt);
	}
}

static int
initializeSharedMem(int cap) {
	int id;

	id = createSharedMem(cap);
	attachToMem(id);

	if (dsInit(nv, cap) == -1)
		pexit("dsInit");

	detachFromMem();

	return id;
}

static void
initializeVarTable() {
	int size;

	size = dsCapToBytes(NV_VT_SIZE);
	vt = malloc(size);
	if (vt == NULL)
		pexit("malloc");

	if (dsInit(vt, NV_VT_SIZE) == -1)
		pexit("dsCreate");
}

static int
createSharedMem(int cap) {
	int id, size;
	size = dsCapToBytes(cap);

	id = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | SHM_PERMS);
	if (id == -1)
		pexit("shmget");

	return id;
}

static void
deleteSharedMem(int id) {
	if (shmctl(id, IPC_RMID, NULL) == -1)
		pexit("shmctl");
}

static long
stringToLong(char *str) {
	char msg[BUF_SIZE];
	char *endptr;
	long n;

	n = strtol(str, &endptr, 10);
	if (n < 0 || n == LONG_MAX || *endptr != '\0') {
		snprintf(msg, BUF_SIZE, "%s: not a valid identifier", str);
		fatal(msg);
	}

	return n;
}

static void
fatal(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

static void
helpAndExit(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "%s - a name/value directory using System V shared memory segments.\n\n", progname);
	fprintf(stream, "Usage:\n\t%s [script-file]\n\n", progname);
	fprintf(stream, "Options:\n");
	fprintf(stream, "\t%10s\t%s\n", "-p", "creates a persistent shared memory segment");
	fprintf(stream, "\t%10s\t%s\n", "-m [id]", "uses a shared memory with the given id");
	fprintf(stream, "\t%10s\t%s. Default: %d\n", "-c [max-nv]", "capacity: maximum number of name/value pairs allowed", MAX_NV_PAIRS);
	fprintf(stream, "\t%10s\t%s\n", "-d [id1]+", "deletes the shared memory with the given ids");
	fprintf(stream, "\t%10s\t%s\n", "-h", "prints this message and exits");

	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
