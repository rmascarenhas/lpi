/* logwtmp.c - an implementation of the `logwtmp(3)` library function.
 *
 * The `wtmp` log file includes an ever growing list of login and logout
 * records for the system.
 *
 * The `logwtmp(3)` function constructs a record for that log file given
 * the data passed as arguments, and updates the `wtmp` log accordingly.
 *
 * Usage
 *
 *    $ ./logout <line> <name> <host> [wtmp_file]
 *    line 		- the name of the controlling terminal of the running shell.
 *    name 		- the username to be recorded. Not validated to exist.
 *    host 		- the name of the host.
 *    wtmp_file - The path for th wtmp log to be updated. Default: _PATH_WTMP
 *
 * Example
 *
 * 	  $ ./logwtmp pts/2 jake localhost /var/log/wtmp
 *
 * After program execution, the `last(1)` command can be used to verify
 * its effect.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _GNU_SOURCE /* updwtmpx */

#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>
#include <utmp.h>
#include <paths.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);

static char *wtmp_file = _PATH_WTMP;

static void _logwtmp(const char *line, const char *name, const char *host);

int
main(int argc, char *argv[]) {
	if (argc < 4)
		helpAndLeave(argv[0], EXIT_FAILURE);

	if (argc > 4)
		wtmp_file = argv[4];

	_logwtmp(argv[1], argv[2], argv[3]);
	printf("Information successfully updated.\n");

	exit(EXIT_SUCCESS);
}

static void
_logwtmp(const char *line, const char *name, const char *host) {
	struct utmpx ut;

	/* copy given data to the record */
	strncpy(ut.ut_line, line, __UT_LINESIZE);
	strncpy(ut.ut_name, name, __UT_NAMESIZE);
	strncpy(ut.ut_host, host, __UT_HOSTSIZE);

	/* use current time */
	time((time_t *) &ut.ut_tv.tv_sec);

	/* use current process ID */
	ut.ut_pid = getpid();

	/* according to the specs, if the `name` parameter is not an empty string,
	 * the record should have type `USER_PROCESS`; otherwise it should be
	 * `DEAD_PROCESS`.
	 *
	 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/baselib-logwtmp-3.html
	 */
	ut.ut_type = strlen(name) == 0 ? DEAD_PROCESS : USER_PROCESS;

	/* use `updwtmp` to update the `wtmp` log file */
	updwtmpx(wtmp_file, &ut);
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s <line> <name> <host> [wtmp_file]\n", progname);
	exit(status);
}
