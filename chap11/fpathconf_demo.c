/* fpathconf_demo.c: demonstration of the fpathconf(3) library function to get some system limits.
 *
 * This is a simple program that demonstrates the use of the `fpathconf(3)` library
 * function to retrieve some filesystem related limits from the system. It's an adapted
 * version of the Listing 11-2 of The Linux Programming Interface book.
 *
 * The information is retrieved from the file referred to by the process standard input.
 *
 * Running on Mavericks (OSX 10.9) using the HFS file system, the output was:
 *
 *    % diskutil info
 *    # snip
 *    File System Personality:  Journaled HFS+
 *    Type (Bundle):            hfs
 *    Name (User Visible):      Mac OS Extended (Journaled)
 *    Journal:                  Journal size 40960 KB at offset 0xe8e000
 *    Owners:                   Enabled
 *    # snip
 *
 *    % ./fpathconf_demo <.
 *    _PC_NAME_MAX: 255
 *    _PC_PATH_MAX: 1024
 *    _PC_PIPE_BUF: 512
 *
 * Running on OpenBSD 5.4 using the FFS filesystem, the output was:
 *
 *    % cat /etc/fstab
 *    /dev/wd0a / ffs rw,softdep,noatime 1 1
 *    # snip
 *
 *    % ./sysconf_demo <.
 *    _PC_NAME_MAX: 255
 *    _PC_PATH_MAX: 1024
 *    _PC_PIPE_BUF: 512
 *
 * Adaptations by: Renato Mascarenhas Costa
 */

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void pexit(const char *fCall);
static void helpAndLeave(const char *progname, int status);
static void fpathconfPrint(const char *msg, int fd, int name);

int
main(int argc, char *argv[]) {
  if (argc != 1) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  fpathconfPrint("_PC_NAME_MAX:", STDIN_FILENO, _PC_NAME_MAX);
  fpathconfPrint("_PC_PATH_MAX:", STDIN_FILENO, _PC_PATH_MAX);
  fpathconfPrint("_PC_PIPE_BUF:", STDIN_FILENO, _PC_PIPE_BUF);

  return EXIT_SUCCESS;
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

static void
fpathconfPrint(const char *msg, int fd, int name) {
  long limit;

  errno = 0;
  limit = fpathconf(fd, name);

  if (limit == -1) {
    /* call succeeded, limit undefined */
    if (errno == 0) {
      printf("%s (indeterminate)\n", msg);
    } else {
      /* error retrieving limit */
      pexit("fpathconf");
    }
  } else {
    /* call succeeded, limit is defined */
    printf("%s %ld\n", msg, limit);
  }
}
