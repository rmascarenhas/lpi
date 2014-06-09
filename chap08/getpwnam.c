/* getpwnam.c - An implementation of getpwnam(3) on top of setpwent(3), getpwent(3) and endpwent(3).
 *
 * The getpwnam(3) library function is used to retrieve information available
 * in the password file (usually on /etc/passwd) about a particular user. This
 * program builds one implementation of this function on top of the setpwent(3),
 * getpwent(3) and endpwent(3) library functions.
 *
 * Usage:
 *
 *    $ ./getpwnam renato
 *    # Available information on user `renato`
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE /* getpwent family of functions */

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);

struct passwd *_getpwnam(const char *name);

int
main (int argc, char *argv[]) {
  char *name;
  struct passwd *info;

  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  name = argv[1];
  errno = 0; /* errno must be set to zero to identify error scenario from non existent user */
  info = _getpwnam(name);

  if (info == NULL) {
    if (errno == 0) {
      printf("User %s does not exist in this system.\n", name);
      exit(EXIT_FAILURE);
    } else {
      pexit("_getpwnam");
    }
  }

  printf("User name: %s\n", info->pw_name);
  printf("Encrypted password: %s\n", info->pw_passwd);
  printf("User ID: %ld\n", (long) info->pw_uid);
  printf("Group ID: %ld\n", (long) info->pw_gid);
  printf("Comment: %s\n", info->pw_gecos);
  printf("Home directory: %s\n", info->pw_dir);
  printf("Login shell: %s\n", info->pw_shell);

  return EXIT_SUCCESS;
}

void
helpAndLeave(const char *progname, int status) {
  fprintf(status == EXIT_FAILURE ? stderr : stdout, "Usage: %s <file>\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

struct passwd*
_getpwnam(const char *name) {
  struct passwd *p;
  long usernameMax;

  usernameMax = sysconf(_SC_LOGIN_NAME_MAX);
  if (usernameMax == -1) {
    usernameMax = 256; /* make a guess */
  }

  setpwent(); /* start scanning from the beginning of the file */
  for (p = getpwent(); p != NULL && strncmp(p->pw_name, name, usernameMax) != 0; p = getpwent());
  endpwent();

  return p;
}
