/* dup_share.c - Proves that duplicated file descriptors share flags and file offset.
 *
 * The dup(2) system call copies the given file descriptor to a new one, returning
 * the new file descriptor number. This program proves that both file descriptors
 * will share the same open flags and file offset.
 *
 * Usage:
 *
 *    $ ./dup_share
 *    # The program will print its steps to prove the shared information
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE /* mkstemp function */

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef DS_WRITE_STR
#define DS_WRITE_STR "dupped file descriptors share information"
#endif

typedef enum { FALSE, TRUE } Bool;

static Bool ok = TRUE;
static char tmpFilePath[] = "/tmp/dup_shareXXXXXX";
static const char writeStr[] = DS_WRITE_STR;

void pexit(const char *fCall);
void safeClose(int fd);

void reportFileOffset(int fd, int newfd);
void writeTo(int fd);

int
main() {
  int fd, newfd;
  int flags, newFlags;

  fd = mkstemp(tmpFilePath);
  if (fd == -1) {
    pexit("mkstemp");
  }

  printf("Created temp file at %s, file descriptor %d\n", tmpFilePath, fd);
  unlink(tmpFilePath);

  newfd = dup(fd);
  printf("Copied file description. File descriptor number is %d\n", newfd);

  flags = fcntl(fd, F_GETFL);
  newFlags = fcntl(newfd, F_GETFL);

  if (flags != newFlags) {
    ok = FALSE;
  }

  printf("Open flags of both files: %d and %d\n", flags, newFlags);
  reportFileOffset(fd, newfd);

  writeTo(fd);
  reportFileOffset(fd, newfd);

  writeTo(newfd);
  reportFileOffset(fd, newfd);

  printf("\nDone. Closing files now.\n");
  safeClose(fd);
  safeClose(newfd);

  if (ok) {
    printf("\nEverything worked as expected. Your system is fine!\n");
    return EXIT_SUCCESS;
  } else {
    printf("\nSomething went wrong. Better check that out.\n");
    return EXIT_FAILURE;
  }
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

void
reportFileOffset(int fd, int newfd) {
  off_t offset, newOffset;

  offset = lseek(fd, 0, SEEK_CUR);
  newOffset = lseek(newfd, 0, SEEK_CUR);

  if (offset != newOffset) {
    ok = FALSE;
  }

  printf("File offset of the files: %ld and %ld\n", (long) lseek(fd, 0, SEEK_CUR), (long) lseek(newfd, 0, SEEK_CUR));
}

void
writeTo(int fd) {
  int numWritten;

  printf("\nWriting to the file pointed by %d\n", fd);
  numWritten = write(fd, writeStr, sizeof(writeStr));
  if (numWritten == -1) {
    pexit("write");
  }

  printf("%d bytes written to the file %d\n", numWritten, fd);
}
