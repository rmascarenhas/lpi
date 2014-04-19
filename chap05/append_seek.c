/* append_seek.c - A test program to demonstrate the behavior of seeking a file open with O_APPEND.
 *
 * The O_APPEND flag tells UNIX to open a file and place the offset pointer to the end of the file.
 * This program demonstrates the behavior of seeking to the start of the file and then writing to it,
 * when the file was open with the O_APPEN flag.
 *
 * Usage:
 *
 *    $ ./append_seek file
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef SA_WRITE_STR
#define SA_WRITE_STR "Writing to the file\n"
#endif

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);

int
main (int argc, char *argv[]) {
  char *filename;
  int flags, fd;
  char writeStr[] = SA_WRITE_STR;
  int numBytes = sizeof(writeStr);
  ssize_t numWritten;

  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  filename = argv[1];
  flags = O_WRONLY | O_APPEND;
  fd = open(filename, flags);

  if (fd == -1) {
    pexit("open");
  }

  lseek(fd, 0, SEEK_SET); /* !!!!!!! has no effect since file was open with O_APPEND !!!!!!! */
  numWritten = write(fd, writeStr, numBytes);

  if (numWritten == -1) {
    pexit("write");
  }

  if (close(fd) == -1) {
    pexit("close");
  }

  printf("Done. Check file %s now. Number of bytes written: %ld\n", filename, (long) numWritten);

  return EXIT_SUCCESS;
}

void
helpAndLeave(const char *progname, int status) {
  fprintf(stderr, "Usage: %s <file>\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
