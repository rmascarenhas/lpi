/* who.c - a very simple implementation of the `who(1)` utility.
 *
 * The `who(1)` command line utility is used to display the usernames
 * of users currently logged-in in the running system.
 *
 * This can be accomplished by scanning the `utmp` system log and
 * looking for records with type `USER_PROCESS`, `INIT_PROCESS` or
 * `LOGIN_PROCESS`. Username and other associated information is
 * also displayed.
 *
 * Example
 *
 * 	  $ ./who [utmp_file]
 * 	  utmp_file - the path to the system utmp file. Default: _PATH_UTMP.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <utmpx.h>
#include <utmp.h>
#include <errno.h>
#include <paths.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>

static void pexit(const char *fname);

int
main(int argc, char *argv[]) {
	if (argc > 1)
		utmpname(argv[1]);

	struct utmpx *ut;

	/* set the file cursor to the start of the file */
	errno = 0;
	setutxent();
	if (errno != 0)
		pexit("setutxent");

	/* iterate over the entries and filter the ones we are interested in */
	while ((ut = getutxent()) != NULL) {
		if (ut->ut_type == INIT_PROCESS || ut->ut_type == LOGIN_PROCESS || ut->ut_type == USER_PROCESS) {
			struct tm *t = localtime((time_t *) &ut->ut_tv.tv_sec);
			char s[32];
			strftime(s, 32, "%F %T", t);

			printf("%12s %10s %20s\n", ut->ut_user, ut->ut_line, s);
		}
	}

	/* finish reading the `utmp` file */
	errno = 0;
	endutxent();
	if (errno != 0)
		pexit("endutxent");

	exit(EXIT_SUCCESS);
}

static void
pexit(const char *fname) {
	perror(fname);
	exit(EXIT_FAILURE);
}
