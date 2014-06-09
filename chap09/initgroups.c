/* initigroups.c - An implementation of the initgroups(3) library function.
 *
 * The initgroups(3) library function sets the supplementary group IDs of the calling
 * process the the groups registered in the groups file for a given user, plus an initial
 * group. It is used mainly by the `login(1)` utility, which sets a couple of process
 * credentials before starting the user's login shell.
 *
 * Usage
 *
 *    $ ./initgroups <username> <groupname>
 *    Supplementary groups are now:
 *    100 - users
 *    2 - wheel
 *    52 - server
 *
 *    <username>  - the user name to be looked for in the groups file
 *    <groupname> - the group name to be also set for the process
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE

#include <unistd.h>
#include <sys/param.h> /* NGROUPS_MAX */
#include <grp.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GROUPS_SIZ (NGROUPS_MAX + 1)

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);

/* Internal function to look for the groups that a given user belongs to by looking
 * through the groups file. The `size` parameter represents the size of the list
 * given in the `groups` parameter.
 *
 * Returns the number of groups the user belongs to on success or -1 on error.
 */
int getGroups(const char *user, int size, gid_t *groups);

/* Internal: returns the id of the group with the given `name`.
 *
 * Returns the group id on success or -1 on error.
 */
gid_t groupIdFromName(const char *name);

/* Internal: returns the name of the group with the given group id `gid`.
 *
 * Return the group name on success or NULL on error.
 */
char *groupNameFromId(gid_t gid);

int _initgroups(const char *user, gid_t group);

int
main(int argc, char *argv[]) {
  char *user, *group;
  gid_t gid, suppGroups[MAX_GROUPS_SIZ];
  int nGroups, i, status;

  if (argc != 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  user = argv[1];
  group = argv[2];
  status = groupIdFromName(group);

  if (status == -1) {
    fprintf(stderr, "Unknown group: %s\n", group);
    exit(EXIT_FAILURE);
  } else {
    gid = status;
  }

  if (_initgroups(user, gid) == -1) {
    pexit("_initgroups");
  }

  if ((nGroups = getgroups(MAX_GROUPS_SIZ, suppGroups)) == -1) {
    pexit("getgroups");
  }

  printf("Supplementary groups are now:\n");
  for (i = 0; i < nGroups; ++i) {
    gid = suppGroups[i];
    group = groupNameFromId(gid);

    printf("%d - %s\n", gid, group);
  }

  exit(EXIT_SUCCESS);
}

void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <username> <groupname>\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

gid_t
groupIdFromName(const char *name) {
  struct group *g;

  if ((g = getgrnam(name)) == NULL) {
    return -1;
  }

  return g->gr_gid;
}

char *
groupNameFromId(gid_t gid) {
  struct group *g;

  if ((g = getgrgid(gid)) == NULL) {
    return NULL;
  }

  return g->gr_name;
}

int
getGroups(const char *user, int size, gid_t *groupsList) {
  struct group *g;
  int n = 0;
  long usernameMax;
  char **p;

  usernameMax = sysconf(_SC_LOGIN_NAME_MAX);
  if (usernameMax == -1) {
    usernameMax = 256; /* make a guess */
  }

  /* point to the beginning of the file to avoid starting on a dirty state */
  setgrent();

  for (g = getgrent(); g != NULL; g = getgrent()) {
    /* does user belong to the group? */
    for (p = g->gr_mem; *p != NULL; ++p) {
      if (strncmp(user, *p, usernameMax) == 0) {
        if (n < size) {
          groupsList[n++] = g->gr_gid;
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

int
_initgroups(const char *user, gid_t group) {
  gid_t suppGroups[MAX_GROUPS_SIZ];
  int nGroups;

  suppGroups[0] = group;

  if ((nGroups = getGroups(user, MAX_GROUPS_SIZ - 1, &suppGroups[1])) == -1) {
    return -1;
  }

  return setgroups(nGroups + 1, suppGroups);
}
