/* realpath.c - an implementation of the realpath(3) library function.
 *
 * This program implements the realpath(3) library function. What it does is
 * taking a path that can be relative, contain symbolic link references, or
 * `.`, `..` or multiple slashes, and then return the canonical path.
 *
 * Usage
 *
 *    $ ./realpath <path>
 *
 *    <name> - the path to be expanded.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 500

#include <limits.h>

#ifndef PATH_MAX
#  include <linux/limits.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef REALPATH_PATH_SEPARATOR
#  define REALPATH_PATH_SEPARATOR ('/')
#  define REALPATH_PATH_SEPARATOR_CSTR ("/")
#endif

static char *_realpath(const char *path, char *resolved_path);

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  char *resolved_path = _realpath(argv[1], NULL);

  if (resolved_path == NULL) {
    pexit("_realpath");
  }

  printf("%s\n", resolved_path);

  free(resolved_path);

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <path>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

static char *
_realpath(const char *path, char *resolved_path) {
  /* sanity checking */
  if (path == NULL) {
    errno = EINVAL;
    return NULL;
  }

  /* this is a GNU extension to SUSv3: if second argument is null, allocate
   * enough space using `malloc`. It is up to the caller to free the returned
   * buffer, though.
   *
   * This behavior is defined in SUSv4 */
  if (resolved_path == NULL) {
    resolved_path = malloc(PATH_MAX);
    if (resolved_path == NULL) {
      return NULL;
    }
  }

  *resolved_path = '\0';

  /* if the first character in the given path is a slash, then it the path is
   * absolute; otherwise it is relative. Note that it is assumed that there is
   * no leading space in the string - they are not handled by this function and
   * should be removed by the caller */
  if (path[0] != REALPATH_PATH_SEPARATOR) {
    if (getcwd(resolved_path, PATH_MAX) == NULL) {
      return NULL;
    }
  }

  struct stat sBuf;
  char *p, *dir, pathdup[PATH_MAX], buf[PATH_MAX], *saveptr;
  int len;

  p = strrchr(resolved_path, REALPATH_PATH_SEPARATOR);

  /* use a thrown-away copy of the given path that we can manipulate */
  strncpy(pathdup, path, PATH_MAX);

  /* add trailing slash, if it does not exist */
  len = strlen(pathdup);

  if (len > 0 && pathdup[len - 1] != REALPATH_PATH_SEPARATOR) {
    pathdup[len] = REALPATH_PATH_SEPARATOR;
    pathdup[len + 1] = '\0';
  }

  dir = strtok_r(pathdup, REALPATH_PATH_SEPARATOR_CSTR, &saveptr);

  while (dir != NULL) {
    if (strncmp(dir, ".", NAME_MAX)) {
      if (!strncmp(dir, "..", NAME_MAX)) {
        /* go up one level */
        *p = '\0';
        p = strrchr(resolved_path, REALPATH_PATH_SEPARATOR);

        /* if there are no more levels to go up, keep at the root level */
        if (p == NULL) {
          *p = REALPATH_PATH_SEPARATOR;
        }

      } else {
        len = strlen(resolved_path);
        resolved_path[len] = REALPATH_PATH_SEPARATOR;
        resolved_path[len + 1] = '\0';

        strncat(resolved_path, dir, PATH_MAX);

        if (lstat(resolved_path, &sBuf) == -1) {
          return NULL;
        }

        if (S_ISLNK(sBuf.st_mode)) {
          /* a link was found: just follow it */
          if (readlink(resolved_path, buf, PATH_MAX) == -1) {
            return NULL;
          }

          int fd;
          fd = open(".", O_RDONLY);
          if (fd == -1) {
            return NULL;
          }

          /* remove the appended dir */
          p = strrchr(resolved_path, REALPATH_PATH_SEPARATOR);
          *p = '\0';

          /* symbolic link locations are calculated related to the path of
           * the link (not to the current process */
          if (chdir(resolved_path) == -1) {
            return NULL;
          }

          /* recursively find the realpath of the link */
          if (_realpath(buf, resolved_path) == NULL) {
            return NULL;
          }

          /* go back to where we were */
          if (fchdir(fd) == -1) {
            return NULL;
          }

          if (close(fd) == -1) {
            return NULL;
          }
        }

        p = strrchr(resolved_path, REALPATH_PATH_SEPARATOR);
      }
    }

    dir = strtok_r(NULL, REALPATH_PATH_SEPARATOR_CSTR, &saveptr);
  }

  return resolved_path;
}
