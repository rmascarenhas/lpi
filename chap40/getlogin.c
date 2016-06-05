/* getlogin.c - an implementation of the `getlogin(3)` library function.
 *
 * The `getlogin(3)` function returns the username of the user currently
 * logged in in the controller terminal of the calling process. Therefore,
 * it fails for daemon processes.
 *
 * This is accomplished by checking the name of the terminal controlling
 * the calling process and then reading the `utmp` file for login information.
 *
 * This function might not work as expected for some terminal emulators.
 * Using a virtual console should be enough to test its functionality.
 *
 * Usage
 *
 *    $ ./getlogin
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <errno.h>
#include <utmpx.h>
#include <utmp.h>
#include <paths.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GETLOGIN_LOGIN_MAX
#  define GETLOGIN_LOGIN_MAX (256)
#endif

#ifndef GETLOGIN_UTMP_FILE
#  define GETLOGIN_UTMP_FILE (_PATH_UTMP)
#endif

static void pexit(const char *fname);
static char *_getlogin(void);

int
main() {
	char *login;

	if ((login = _getlogin()) == NULL)
		pexit("_getlogin");

	printf("%s\n", login);
	exit(EXIT_SUCCESS);
}

static char *
_getlogin() {
	/* find name of the controlling terminal of the calling process */
	char *tty;

	/* if the controlling terminal name cannot be determined, return a NULL pointer.
	 * `errno` will have been appropriately set by `ttyname(3)`. */
	if ((tty = ttyname(STDIN_FILENO)) == NULL)
		return NULL;

	/* the static data structure used for all calls to this function */
	static char login[GETLOGIN_LOGIN_MAX];

	utmpname(GETLOGIN_UTMP_FILE);

	/* search for an entry in the utmp file where the ut_line field matches
	 * that of the controlling terminal name */
	errno = 0;
	setutxent();
	if (errno != 0)
		return NULL;

	struct utmpx *entry, criteria;
	char *line;

	/* drop the leading slash, if any */
	if (tty[0] == '/')
		++tty;

	/* remove the `dev/` prefix from the ttyname, if existent */
	if ((line = strchr(tty, '/')) == NULL)
		line = tty;
	else
		/* dev/pts/0 becomes pts/0 */
		++line;

	strncpy(criteria.ut_line, line, __UT_LINESIZE);
	if ((entry = getutxline(&criteria)) == NULL)
		return NULL;

	strncpy(login, entry->ut_user, GETLOGIN_LOGIN_MAX);

	/* finish the reading of the utmp file */
	errno = 0;
	endutxent();
	if (errno != 0)
		return NULL;

	return login;
}

static void
pexit(const char *fname) {
	perror(fname);
	exit(EXIT_FAILURE);
}
