/* atomic_append.c - Demonstrate the atomocity provided by writes in files open with O_APPEND.
 *
 * When the O_APPEND flag is specified when opening a file, UNIX guarantees that
 * writes to that file will happen at the end of it, providing atomic cursor
 * positioning and actual writing. This program demonstrates this behavior by
 * constrasting writing to a file open with O_APPEND vs. writing to a file without
 * the flag, but seeking to the end before writing.
 *
 * Usage:
 *
 *    $ ./atomic_append <file> <numBytes> [x]
 *
 *    file - the test file that should be written to demonstrate the issue.
 *    numBytes - the number of bytes that will be written, one at a time.
 *    [x] (optional) - if given a third argument (anything), instead of opening
 *    the file with O_APPEND, the program will perform an lseek to try to write
 *    at the end of the file.
 *
 * Examples:
 *
 *    $ ./atomic_append file.txt 10000
 *    # writes 10000 bytes using O_APPEND.
 *
 *    $ ./atomic_append file.txt 10000 x
 *    # writes 10000 using lseek and then write.
 *
 *
 * This program demonstrate the race conditions that arise when multiple writes
 * occur to the same file concurrently. Running this program as in:
 *
 *    $ ./atomic_append f1 1000000 & ./atomic_append 1000000 f1
 *    $ ./atomic_append f1 1000000 x & ./atomic_append 1000000 f2 x
 *
 * Results in the following files (may be different in other runs of the program):
 *
 *    -rw-rw-rw- 1 renato 2000000 Mar 16 21:55 f1
 *    -rw-rw-rw- 1 renato 1948882 Mar 16 21:52 f2
 *
 * As can be seen, the file without the O_APPEND flag did not have atomic writes,
 * which caused some of the writes to be done when the file offset was not actually
 * at the end of the file, due to race conditions (process scheduler stopped execution
 * between lseek(2) and write(2) calls).
 *
 * For this reason, to be sure a write(2) call will write to the end of the file without
 * overriding existing content, the O_APPEND flag must be passed.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef AA_BYTE
#define AA_BYTE 'a'
#endif

typedef enum { FALSE, TRUE } Bool;

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);
int writeBytes(int fd, int numBytes, Bool useAppend);

static const char byte[] = { AA_BYTE };

int
main(int argc, char *argv[]) {
  int fd, flags, numBytes, numWritten;
  char *filename;
  mode_t mode;
  Bool useAppend;

  if (argc < 3 || argc > 4) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  if (argc == 3) {
    useAppend = TRUE;
  } else {
    useAppend = FALSE;
  }

  filename = argv[1];
  numBytes = (int) atol(argv[2]);

  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; /* rw-rw-rw- */
  flags = O_WRONLY | O_CREAT;

  if (useAppend) {
    flags |= O_APPEND;
  }

  fd = open(filename, flags, mode);
  if (fd == -1) {
    pexit("open");
  }

  numWritten = writeBytes(fd, numBytes, useAppend);

  if (close(fd) == -1) {
    pexit("close");
  }

  printf("Done. Written %ld bytes to the file %s.\n", (long) numWritten, filename);

  return EXIT_SUCCESS;
}

int
writeBytes(int fd, int numBytes, Bool useAppend) {
  int i, numWritten, totalWritten = 0;

  for (i = 0; i < numBytes; ++i) {
    if (!useAppend) {
      lseek(fd, 0, SEEK_END);
    }

    numWritten = write(fd, byte, 1);

    if (numWritten == -1) {
      pexit("write");
    }

    totalWritten += numWritten;
  }

  return totalWritten;
}

void
helpAndLeave(const char *progname, int status) {
  fprintf(stderr, "Usage: %s <file> <numBytes> [x]\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
