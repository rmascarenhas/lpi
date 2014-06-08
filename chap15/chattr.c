/* chattr.c - a simple implementation of the chattr(1) utility.
 *
 * The chattr(1) command allows the user to change extended attributes for a file
 * maintained by the filesystem. The feature was introduced on ext2 and is a Linux
 * extension, although modern BSDs have equivalent functionality.
 *
 * The list of supported attributes are:
 *
 *    a - append only
 *    c - enable compression
 *    D - synchronous directory updates
 *    i - immutable
 *    j - enable data journaling
 *    A - don't update file last access time
 *    d - no dump
 *    t - no tail packing
 *    s - secture deletion
 *    S - synchronous file (and directory) updates
 *    T - treat as top-level directory for Orlov
 *    u - file can be undeleted
 *
 * Note that this program will only attempt the requested file attributes.
 * Actual support for their functionality is conditioned to the running
 * kernel version.
 *
 * Usage:
 *
 *    $ ./chattr <+-=><flags> <file> [<file2> <file3> ...]
 *
 *    - + = indicate if the given flags should be removed, added or set
 *    (removing previously set flags)
 *
 *    <flags> - one or more of the flags listed above
 *
 *    <files1..N> - the files to have attributes changed
 *
 * Examples:
 *
 *    $ ./chattr +i file   # adds immutable flag to file
 *    $ ./chattr -jd file  # removes flags j and d from file
 *    $ ./chattr =acD file # sets the extended attributes to exactly a, c and D
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/fs.h>

#include <stdio.h>
#include <stdlib.h>

typedef enum { RM, ADD, SET } chtype;

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  if (argc < 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  int attrs, current_attrs, fd, i;
  char *p;
  chtype type;

  attrs = 0;
  p = argv[1];

  switch (p[0]) {
    case '-': type = RM;                           break;
    case '+': type = ADD;                          break;
    case '=': type = SET;                          break;
    default:  helpAndLeave(argv[0], EXIT_FAILURE); break;
  }

  for (p = argv[1] + 1; *p != '\0'; ++p) {
    switch (*p) {
      case 'a': attrs |= FS_APPEND_FL;                break;
      case 'c': attrs |= FS_COMPR_FL;                 break;
      case 'D': attrs |= FS_DIRSYNC_FL;               break;
      case 'i': attrs |= FS_IMMUTABLE_FL;             break;
      case 'j': attrs |= FS_JOURNAL_DATA_FL;          break;
      case 'A': attrs |= FS_NOATIME_FL;               break;
      case 'd': attrs |= FS_NODUMP_FL;                break;
      case 't': attrs |= FS_NOTAIL_FL;                break;
      case 's': attrs |= FS_SECRM_FL;                 break;
      case 'S': attrs |= FS_SYNC_FL;                  break;
      case 'T': attrs |= FS_TOPDIR_FL;                break;
      case 'u': attrs |= FS_UNRM_FL;                  break;
      default:  helpAndLeave(argv[0], EXIT_FAILURE);  break;
    }
  }

  for (i = 2; i < argc; ++i) {
    fd = open(argv[i], O_RDONLY);
    if (fd == -1) {
      pexit("open");
    }

    if (ioctl(fd, FS_IOC_GETFLAGS, &current_attrs) == -1) {
      pexit("ioctl");
    }

    switch (type) {
      case ADD: attrs |= current_attrs;             break;
      case RM:  attrs = current_attrs & ~attrs;     break;
      case SET: /* just set the given attributes */ break;
    }

    if (ioctl(fd, FS_IOC_SETFLAGS, &attrs) == -1) {
      pexit("ioctl");
    }

    if (close(fd) == -1) {
      pexit("close");
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

  fprintf(stream, "Usage: %s <-+=><flags> <file> [<file2> <file3 ...]\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
