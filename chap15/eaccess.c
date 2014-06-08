/* eaccess.c - an implementation of the access(2) system call based on effective IDs.
 *
 * The access(2) system call checks for a user's permissions on a file based on the
 * operation on a file is performed. However, the test is useful to determine
 * the invoking user's permissions on set-user-ID programs.
 *
 * This program implements a similar feature as provided by access(2), but using
 * effective credentials.
 *
 * Usage:
 *
 *    $ ./eaccess <frwx> <file>
 *    file: permission granted
 *
 *    <frwx> - specify as many flags as wanted for the access check:
 *        f - checks if the file exist (F_OK)
 *        r - checks for read permission (R_OK)
 *        w - checks for write permission (W_OK)
 *        x - checks for execute permission (X_OK)
 *
 *    <file> - the file to be checked against
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

static int eaccess(const char *pathname, int mode);

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  if (argc != 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  char *accessString, *filename, *p;
  int mode;

  accessString = argv[1];
  filename = argv[2];
  mode = 0;

  for (p = accessString; *p != '\0'; ++p) {
    switch (*p) {
      case 'f': mode |= F_OK; break;
      case 'r': mode |= R_OK; break;
      case 'w': mode |= W_OK; break;
      case 'x': mode |= X_OK; break;
      default:
        fprintf(stderr, "%s: unknown flag %c\n", argv[0], *p);
        exit(EXIT_FAILURE);
    }
  }

  errno = 0;
  if (eaccess(filename, mode) == 0) {
    printf("%s: permission granted\n", filename);
  } else {
    if (errno != 0) {
      pexit("eaccess");
    }

    printf("%s: permission denied\n", filename);
  }

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <frwx> <file>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

/* Implement the permission check algorithm to determine if a file is accessible,
 * using the effective credentials:
 *
 *  * If the process owner is the owner of the file, owner permissions are checked;
 *  * If it is not the owner, but belongs to the owner group, group permissions
 *    are checked;
 *  * If none of the above, then the `other' permissions apply.
 */
static int
eaccess(const char *pathname, int mode) {
  struct stat info;

  if (stat(pathname, &info) == -1) {
    return -1;
  }

  uid_t euid, egid;
  int r, w, x;

  euid = geteuid();
  egid = getegid();

  if (info.st_uid == euid) {
    /* owner of the file */
    r = info.st_mode & S_IRUSR;
    w = info.st_mode & S_IWUSR;
    x = info.st_mode & S_IXUSR;
  } else if (info.st_gid == egid) {
    /* belongs to the group owner */
    r = info.st_mode & S_IRGRP;
    w = info.st_mode & S_IWGRP;
    x = info.st_mode & S_IXGRP;
  } else {
    /* apply `other' permissions */
    r = info.st_mode & S_IROTH;
    w = info.st_mode & S_IWOTH;
    x = info.st_mode & S_IXOTH;
  }

  if (((mode & R_OK) && !r) || ((mode & W_OK) && !w) || ((mode & X_OK) && !x)) {
    return -1;
  }

  return 0;
}
