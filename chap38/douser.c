/* douser.c - a simple implementation of the `sudo(1)` utility.
 *
 * Sometimes it is necessary to run a certain program as a different
 * user (or as root) in order to gain or drop privileges. This is the
 * purpose of this program.
 *
 * In order to run a given program as a certain user, this programs
 * authenticates the invoker to then change the process user and group
 * IDs to those of the requested user.
 *
 * Usage
 *
 *    $ ./douser [-u user] command
 *
 *    -u - the username of the user to run the given command as. Defaults to root.
 *
 * Note: as this program requires access to the password and shadow files, it must
 * either be run as root or as set-user-ID-root (common approach for `sudo` on many
 * platforms.)
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE            /* crypt() and getopt() function */
#define _XOPEN_SOURCE_EXTENDED   /* getpass() function            */
#define _ISOC99_SOURCE           /* snprintf() function           */
#define _DEFAULT_SOURCE          /* setgroups() function          */

#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <shadow.h>
#include <grp.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PWD_LEN (1024)
#define DEFAULT_USERNAME ("root")

#ifndef BUFSIZ
#  define BUFSIZ (1024)
#endif

/* make a guess if the constant was not defined in the headers above */
#ifndef NGROUPS_MAX
#  define NGROUPS_MAX (1024)
#endif

static void fatal(const char *message);
static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static bool authenticate(const char *username, uid_t *uid, gid_t *gid);
static int get_groups_for_user(const uid_t uid, int size, gid_t *groups);

int
main(int argc, char *argv[]) {
	if (argc < 2)
		helpAndLeave(argv[0], EXIT_FAILURE);

	int opt;
	int num_groups;
	uid_t uid;
	gid_t gid;
	gid_t groups[NGROUPS_MAX];
	char *file;
	char *username = DEFAULT_USERNAME;

	/* command-line parsing */
	opterr = 0;
	while ((opt = getopt(argc, argv, "+u:")) != -1) {
		switch(opt) {
			case 'u': username = optarg; break;
			case '?': helpAndLeave(argv[0], EXIT_FAILURE); break;
		}
	}

	/* no command given */
	if (optind == argc)
		helpAndLeave(argv[0], EXIT_FAILURE);

	if (!authenticate(username, &uid, &gid))
		fatal("Authentication failure");

	/* step 1. get supplementary groups for the requested user */
	if ((num_groups = get_groups_for_user(uid, NGROUPS_MAX, groups)) == -1)
		pexit("get_groups_for_user");

	/* step 2. assign the current process those groups. */
	if (setgroups(num_groups, groups) == -1)
		pexit("setgroups");

	/* step 3. Change the real and effective user IDs to those of the requested user */
	if (setreuid(uid, uid) == -1)
		pexit("setreuid");

	/* step 4. Change the real and effective group IDs to those of the requested user */
	if (setregid(gid, gid) == -1)
		pexit("setregid");

	/* step 5. exec the given command */
	file = argv[optind];
	if (execvp(file, &argv[optind]) == -1)
		pexit("execvp");

	/* never reached */
	exit(EXIT_SUCCESS);
}

static bool
authenticate(const char *username, uid_t *uid, gid_t *gid) {
	char *password, *encrypted, *p;
	struct passwd *pwd;
	struct spwd *spwd;
	char message[BUFSIZ];

	pwd = getpwnam(username);
	if (pwd == NULL)
		fatal("Could not read password record. Double check that the username was entered correctly.");

	spwd = getspnam(username);
	if (spwd == NULL && errno == EACCES)
		fatal("No permission to read the shadow password file.");

	/* if there is a shadow password record for the given user, use it
	 * instead */
	if (spwd != NULL)
		pwd->pw_passwd = spwd->sp_pwdp;

	snprintf(message, BUFSIZ, "[douser] password for %s: ", username);
	password = getpass(message);

	/* encrypt password and delete plain-text version immediately */
	encrypted = crypt(password, pwd->pw_passwd);
	for (p = password; *p != '\0'; ++p) {
		*p = '\0';
	}

	if (encrypted == NULL)
		pexit("crypt");

	if (strncmp(encrypted, pwd->pw_passwd, MAX_PWD_LEN) == 0) {
		*uid = pwd->pw_uid;
		*gid = pwd->pw_gid;
		return true;
	}

	return false;
}

/* given a `username`, this function returns the user ID associated with
 * that. Bails out if the username is not found. */
static uid_t
user_id_from_name(const char *username) {
	errno = 0;
	struct passwd *p;

	p = getpwnam(username);
	if (errno != 0) {
		pexit("getpwnam");
	} else if (p == NULL) {
		fprintf(stderr, "unknown user: %s\n", username);
		exit(EXIT_FAILURE);
	}

	return p->pw_uid;
}

/* parses the groups file of the running system to determine all groups
 * of a given user (identified by the given `uid`.)
 *
 * The result is populated in the `groups` list, which is of size `size`,
 * given as parameter.
 * */
static int
get_groups_for_user(const uid_t uid, int size, gid_t *groups) {
	struct group *g;
	uid_t cuid; /* current user ID */
	int n = 0;
	char **p;

	/* point to the beginning of the group files to make sure the navigation
	 * starts on a clean state */
	setgrent();

	/* the structure returned by `getgrent` contains a list of usernames that
	 * belong to the current group. However, we only have access to the user ID
	 * here. We need to fetch the user ID related to each declared username
	 * and check if it matches the given user ID. */
	for (g = getgrent(); g != NULL; g = getgrent()) {
		for (p = g->gr_mem; *p != NULL; ++p) {
			cuid = user_id_from_name(*p);
			if (cuid == uid) {
				if (n < size) {
					groups[n++] = g->gr_gid;

					/* if we find out the user is a member of the current group, there
					 * is no need to check the remaining members */
					break;
				} else {
					errno = ENOMEM;
					return -1;
				}
			}
		}
	}

	endgrent();

	return n;
}

static void
fatal(const char *message) {
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS) {
		stream = stdout;
	}

	fprintf(stream, "Usage: %s [-u user] command\n", progname);
	exit(status);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
