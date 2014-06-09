/* dup_dup2.c - Implement the dup(2) and dup2(2) system calls on top of fcntl(2).
 *
 * The dup(2) and dup2(2) system calls are used create a file descriptor that refers
 * to a previous file description. UNIX guarantees that the returned file descriptor
 * number is the smallest available. If you want to enforce a new value for the
 * new file descriptor, you can use dup(2), which will create a copy with that file
 * descriptor number.
 *
 * This program copies two file descriptors given on the command line and write to them
 * (configurable at compilation time) to ensure the copy went fine.
 *
 * Usage:
 *
 *    $ ./dup_dup2 <oldfd> [newfd]
 *    # Print success or failure message depending on result.
 *
 *    oldfd - the file descriptor number to be copied.
 *    newfd (optional) - if given, the program will use its version of dup2 to
 *    create a new file descriptor with that number.
 *
 * The program will write a few bytes (that can be configured at compilation time) to
 * the new file descriptor, to ensure the copy happened successfully.
 *
 * Examples
 *
 *    $ ./dup_dup2 5>file.txt 5
 *    $ ./dup_dup2 5>file.txt 5 10
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef DD_WRITE_STR
#define DD_WRITE_STR "Writing to the copied file descriptor\n"
#endif

typedef enum { FALSE, TRUE } Bool;

static const char writeStr[] = DD_WRITE_STR;

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);
void safeClose(int fd);

int _dup(int oldfd);
int _dup2(int oldfd, int newfd);

int duplicate(int fd, int newfd, Bool useDup2);

int
main(int argc, char *argv[]) {
  int fd, newfd;
  int numWritten;
  Bool useDup2;

  if (argc < 2 || argc > 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  fd = (int) atol(argv[1]);
  newfd = -1; /* if not using dup2, this variable will not be used */

  if (argc == 3) {
    useDup2 = TRUE;
    newfd = (int) atol(argv[2]);
  } else {
    useDup2 = FALSE;
  }

  newfd = duplicate(fd, newfd, useDup2);

  numWritten = write(newfd, writeStr, sizeof(writeStr));
  if (numWritten == -1) {
    pexit("write");
  }

  safeClose(fd);
  safeClose(newfd);

  printf("Done. Written %d bytes to the new file descriptor #%d\n", numWritten, newfd);

  return EXIT_SUCCESS;
}

int
duplicate(int fd, int newfd, Bool useDup2) {
  if (useDup2) {
    return _dup2(fd, newfd);
  }

  return _dup(fd);
}

int
_dup(int oldfd) {
  int newfd = fcntl(oldfd, F_DUPFD);

  if (newfd == -1) {
    pexit("fcntl");
  }

  return newfd;
}

int
_dup2(int oldfd, int newfd) {
  int nextfd;

  /* First, check if oldfd is a valid file descriptor */
  int flags = fcntl(oldfd, F_GETFL);
  if (flags == -1) {
    errno = EBADF;
    return -1;
  }

  /* if both descriptors are the same, do nothing */
  if (oldfd == newfd) {
    return newfd;
  }

  /* close newfd if necessary */
  flags = fcntl(newfd, F_GETFL);
  if (flags != -1) {
    safeClose(newfd);
  }

  /* perform the copy */
  nextfd = fcntl(oldfd, F_DUPFD, newfd);

  if (nextfd != newfd) {
    fprintf(stderr, "_dup2 is not atomic, so sometimes it fails. It just did. Expected fd %d, got %d\n", newfd, nextfd);
    exit(EXIT_FAILURE);
  }

  return newfd;
}

void
helpAndLeave(const char *progname, int status) {
  FILE *stream;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  } else {
    stream = stderr;
  }

  fprintf(stream, "Usage: %s <oldfd> [newfd]\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

void
safeClose(int fd) {
  if (close(fd) == -1) {
    pexit("close");
  }
}
