/* getcwd.c - an implementation of the getcwd(3) library function.
 *
 * The `getcwd(3)` library function provides a way to retrieve the current
 * working directory of a process.
 *
 * The way this implementation works is getting the current directory name from
 * the parent directory, scanning its entries until one with the same i-node and
 * device number is found. This is the algorithm suggested by ``The Linux
 * Programming Interface'' book.
 *
 * Usage
 *
 *    $ ./getcwd
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 500

#include <limits.h>

#ifndef PATH_MAX
#  include <linux/limits.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef enum { FALSE, TRUE } Bool;

static char *_getcwd(char *buf, size_t size);
static void pexit(const char *fCall);

int
main() {
  char cwd[PATH_MAX];

  if (_getcwd(cwd, PATH_MAX) == NULL) {
    pexit("_getcwd");
  }

  printf("%s\n", cwd);

  exit(EXIT_SUCCESS);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

static char *
_getcwd(char *buf, size_t size) {
  size_t total;
  int fd;
  struct stat sCur, sParent, sEntry;
  char parentDir[NAME_MAX], curFile[NAME_MAX];
  DIR *dir;
  struct dirent *entry;
  Bool found;

  if (stat(".", &sCur) == -1) {
    return NULL;
  }

  if (stat("..", &sParent) == -1) {
    return NULL;
  }

  /* recursion stop condition */
  if (sCur.st_dev == sParent.st_dev && sCur.st_ino == sParent.st_ino) {
    snprintf(buf, size, "/");
    return buf;
  }

  /* open the parent dir and scan it for the current directory name */
  dir = opendir("..");
  if (dir == NULL) {
    return NULL;
  }

  found = FALSE;
  errno = 0;
  entry = readdir(dir);

  while (!found && entry != NULL) {
    /* skip . and .. entries */
    if (strncmp(entry->d_name, ".", NAME_MAX) && strncmp(entry->d_name, "..", NAME_MAX)) {
      snprintf(curFile, NAME_MAX, "../%s", entry->d_name);

      if (stat(curFile, &sEntry) == -1) {
        return NULL;
      }

      if (sEntry.st_dev == sCur.st_dev && sEntry.st_ino == sCur.st_ino) {
        found = TRUE;
        continue;
      }
    }
    entry = readdir(dir);
  }

  if (errno != 0) {
    return NULL;
  }

  if (entry == NULL) {
    return NULL;
  } else {
    total = strlen(entry->d_name);
  }

  /* recursively find the path to the parent directory */
  fd = open(".", O_RDONLY);
  if (fd == -1) {
    return NULL;
  }

  if (chdir("..") == -1) {
    return NULL;
  }

  if (_getcwd(parentDir, NAME_MAX) == NULL) {
    return NULL;
  }

  total += strlen(parentDir);
  if (total >= size) {
    errno = ERANGE;
    return NULL;
  }

  /* generate the current working directory by concatening the result found
   * when running recursively on the parent and the current dir name */
  if (strncmp(parentDir, "/", NAME_MAX)) {
    snprintf(buf, size, "%s/%s", parentDir, entry->d_name);
  } else {
    snprintf(buf, size, "/%s", entry->d_name);
  }

  /* get back to where we were */
  if (fchdir(fd) == -1) {
    return NULL;
  }

  if (close(fd) == -1) {
    return NULL;
  }

  if (closedir(dir) == -1) {
    return NULL;
  }

  return buf;
}
