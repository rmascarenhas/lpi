/* logout.c - an implementation of the `logout(3)` library function.
 *
 * Whenever a user logs into a system, the login-providing program should
 * update the login accounting records - namely the `/var/run/utmp` and
 * the `/var/log/wtmp` files.
 *
 * The `logout(3)` library function is responsible for removing a previously
 * added entry on the utmp file, indicating that a user is no longer
 * logged in the system.
 *
 * Usage
 *
 *    $ ./logout <ut_line> [utmp_file] [wtmp_file]
 *    ut_line   - the name of the controlling terminal of the running shell.
 *    utmp_file - the path for the utmp file to be used. Default: _PATH_UTMP
 *    wtmp_file - the path for the wtmp file to be used. Default: _PATH_WTMP
 *
 * Example
 *
 * 	  $ ./logout pts/0 /var/run/utmp /var/log/wtmp
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
#include <errno.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fname);

static int _logout(const char *ut_line);

static char *utmp_file = _PATH_UTMP,
			*wtmp_file = _PATH_WTMP;

int
main(int argc, char *argv[]) {
	if (argc == 1)
		helpAndLeave(argv[0], EXIT_FAILURE);

	char *ut_line  = argv[1];

	if (argc > 2)
		utmp_file = argv[2];

	if (argc > 3)
		wtmp_file = argv[3];

	utmpname(utmp_file);
	if (_logout(ut_line) == 0)
		pexit("_logout");

	printf("%s has been logged out.\n", ut_line);
	exit(EXIT_SUCCESS);
}

static int
_logout(const char *ut_line) {
	struct utmpx ut,
				*entry;

	strncpy(ut.ut_line, ut_line, __UT_LINESIZE);

	errno = 0;
	setutxent();
	if (errno != 0)
		return 0;

	/* if an entry for the given line is found, erase it.
	 * Otherwise, just return successfully. */
	if ((entry = getutxline(&ut)) == NULL)
		return 1;

	/* erase ut_user and ut_host fields. */
	memset(&entry->ut_user, 0, sizeof(entry->ut_user));
	memset(&entry->ut_host, 0, sizeof(entry->ut_host));

	/* update timestamp */
	time((time_t *) &entry->ut_tv.tv_sec);

	/* indicates that a logout is happening */
	entry->ut_type = DEAD_PROCESS;

	errno = 0;
	setutxent();
	if (errno != 0)
		return 0;

	/* commit the change to the utmp file */
	if (pututxline(entry) == NULL)
		return 0;

	errno = 0;
	endutxent();
	if (errno != 0)
		return 0;

	/* commit the change to the wtmp file */
	updwtmpx(wtmp_file, entry);

	return 1;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s <ut_line> [utmp_file] [wtmp_file]\n", progname);
	exit(status);
}

static void
pexit(const char *fname) {
	perror(fname);
	exit(EXIT_FAILURE);
}
