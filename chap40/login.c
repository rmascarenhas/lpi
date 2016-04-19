/* login.c - an implementation of the `login(3)` library function.
 *
 * Whenever a user logs into a system, the login-providing program should
 * update the login accounting records - namely the `/var/run/utmp` and
 * the `/var/log/wtmp` files.
 *
 * The `login(3)` library function is responsible for updating the `/var/run/utmp`
 * file with the information of a user that has just logged in.
 *
 * Usage
 *
 *    $ ./login <username> [utmp_file] [wtmp_file]
 *    username  - the username to be logged in (not validated to exist)
 *    utmp_file - the path for the utmp file to be used. Default: _PATH_UTMP
 *    wtmp_file - the path for the wtmp file to be used. Default: _PATH_WTMP
 *
 * After program execution, the user identified by the given username will be
 * have an entry on the utmp file. The `last(1)` command can be used to verify
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fname);

static void _login(const struct utmpx *ut);

char *utmp_file = _PATH_UTMP,
	 *wtmp_file = _PATH_WTMP;

int
main(int argc, char *argv[]) {
	if (argc == 1)
		helpAndLeave(argv[0], EXIT_FAILURE);

	char *username  = argv[1];

	if (argc > 2)
		utmp_file = argv[2];

	if (argc > 3)
		wtmp_file = argv[3];

	utmpname(utmp_file);
	struct utmpx ut;
	strncpy(ut.ut_user, username, __UT_NAMESIZE);

	_login(&ut);

	printf("Username %s has been logged in.\n", username);
	exit(EXIT_SUCCESS);
}

static void
_login(const struct utmpx *ut) {
	struct utmpx record;
	strncpy(record.ut_user, ut->ut_user, __UT_NAMESIZE);

	/* fill in basic information */
	record.ut_type = USER_PROCESS;
	record.ut_pid  = getpid();

	int stds[] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO },
		i;
	char *tty;

	/* iterate over stdin, stdout and stderr (in that order), trying to find the
	 * name of the controlling terminal of the calling process. */
	for (i = 0; i < 2; ++i) {
		if ((tty = ttyname(stds[1])) != NULL)
			break;
	}

	if (tty != NULL) {
		/* strip leading slash if any */
		if (tty[0] == '/')
			++tty;

		char *p;
		/* remove `dev/` prefix */
		if ((p = strchr(tty, '/')) != NULL)
			tty = p + 1;

		strncpy(record.ut_line, tty, __UT_LINESIZE);
	}

	/* commit the record to the utmp file */
	if (pututxline(&record) == NULL)
		pexit("pututxline");

	/* commit the record to the wtmp file */
	updwtmpx(wtmp_file, &record);
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s <username> [utmp_file] [wtmp_file]\n", progname);
	exit(status);
}

static void
pexit(const char *fname) {
	perror(fname);
	exit(EXIT_FAILURE);
}
