/* sysconf_demo.c: demonstration of the sysconf(3) library function to get some system limits.
 *
 * This is a simple program that demonstrates the use of the `sysconf(3)` library
 * function to retrieve some system limits. It's an adapted version of the
 * Listing 11-1 of The Linux Programming Interface book.
 *
 * Running on Mavericks (OSX 10.9), the output was:
 *
 *    % uname -a
 *    Darwin kraken-2.local 13.1.0 Darwin Kernel
 *    Version 13.1.0: Wed Apr  2 23:52:02 PDT 2014; root:xnu-2422.92.1~2/RELEASE_X86_64 x86_64
 *
 *    % ./sysconf_demo
 *   _SC_ARG_MAX:        262144
 *   _SC_LOGIN_NAME_MAX: 255
 *   _SC_OPEN_MAX:       256
 *   _SC_NGROUPS_MAX:    16
 *   _SC_PAGESIZE:       4096
 *   _SC_RTSIG_MAX:      (indeterminate)
 *
 * Running on OpenBSD 5.4, the output was:
 *
 *    % uname -a
 *    OpenBSD 5.4 GENERIC.MP#1 i386
 *
 *    % ./sysconf_demo
 *    _SC_ARG_MAX:        262144
 *    _SC_LOGIN_NAME_MAX: 32
 *    _SC_OPEN_MAX:       50
 *    _SC_NGROUPS_MAX:    16
 *    _SC_PAGESIZE:       4096
 *    sysconf: Invalid argument
 *
 * As we can see from the output above, OpenBSD errors out when given the
 * _SC_RTSIG_MAX sysconf name.
 *
 * Adaptations by: Renato Mascarenhas Costa
 */

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void pexit(const char *fCall);
static void helpAndLeave(const char *progname, int status);
static void sysconfPrint(const char *msg, int name);

int
main(int argc, char *argv[]) {
  if (argc != 1) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  sysconfPrint("_SC_ARG_MAX:       ", _SC_ARG_MAX);
  sysconfPrint("_SC_LOGIN_NAME_MAX:", _SC_LOGIN_NAME_MAX);
  sysconfPrint("_SC_OPEN_MAX:      ", _SC_OPEN_MAX);
  sysconfPrint("_SC_NGROUPS_MAX:   ", _SC_NGROUPS_MAX);
  sysconfPrint("_SC_PAGESIZE:      ", _SC_PAGESIZE);
  sysconfPrint("_SC_RTSIG_MAX:     ", _SC_RTSIG_MAX);

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
sysconfPrint(const char *msg, int name) {
  long limit;

  errno = 0;
  limit = sysconf(name);

  if (limit == -1) {
    /* call succeeded, limit undefined */
    if (errno == 0) {
      printf("%s (indeterminate)\n", msg);
    } else {
      /* error retrieving limit */
      pexit("sysconf");
    }
  } else {
    /* call succeeded, limit is defined */
    printf("%s %ld\n", msg, limit);
  }
}
