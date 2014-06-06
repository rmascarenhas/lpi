/* setfattr.c - sets user extended attributes for a file.
 *
 * This program can be viewed as a much simpler version of setfattr(1) utility,
 * used to set, remove and list a file's extended attributes. All this program
 * can do, however, is to set user EAs.
 *
 * Usage
 *
 *    $ ./setfattr <name> <value> <file>
 *
 *    <name>  - the name of the EA to be set. Note that the `user.` namespace
 *    is added automatically.
 *    <value> - the value to be set.
 *    <file>  - the file to which the program should add the EA.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  if (argc != 4) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  char ea_name[BUFSIZ];
  char *name, *value, *file;

  name  = argv[1];
  value = argv[2];
  file  = argv[3];

  snprintf(ea_name, BUFSIZ, "user.%s", name);

  if (setxattr(file, ea_name, value, strlen(ea_name), 0) == -1) {
    pexit("setxattr");
  }

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <name> <value> <file>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
