/* chmod_arx.c - the equivalent of chmod a+rX on the command line.
 *
 * The chmod(1) utility uses the chmod(2) system call to perform a series of
 * changes on file permissions. Particularly, if you invoke it as in:
 *
 *    $ chmod a+rX <files>
 *
 * it will give read permission for every type of user, and will give execute
 * permission:
 *
 *    * for every type of user if the file is a directory;
 *    * for every type of user if the file was already executable by some user.
 *
 * Usage:
 *
 *    $ ./chmod_arx <file1> ... <fileN>
 *
 *    <file1..N> - the files to have their permissions changes
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  if (argc == 1) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  struct stat info;
  mode_t mode;
  int i;

  for (i = 1; i < argc; ++i) {
    if (stat(argv[i], &info) == -1) {
      pexit("stat");
    }

    /* every type of user gains read access */
    mode = info.st_mode;
    mode |= S_IRUSR | S_IRGRP | S_IROTH;

    if (S_ISDIR(info.st_mode)) {
      /* if file is a directory, everyone gains search permissions */
      mode |= S_IXUSR | S_IXGRP | S_IXOTH;
    } else {
      /* file is not a directory: only give execute permissions to everyone
       * if at least one type of user is able to execute it*/
      if ((info.st_mode & S_IXUSR) || (info.st_mode & S_IXGRP) || (info.st_mode & S_IXOTH)) {
        mode |= S_IXUSR | S_IXGRP | S_IXOTH;
      }
    }

    if (chmod(argv[i], mode) == -1) {
      pexit("chmod");
    }
  }

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <file> [<file2> <file3> ...]\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
