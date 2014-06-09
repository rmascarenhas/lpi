/* readv_writev.c - An implementation of the readv(2) and writev(2) system calls on
 *                  top of read(2) and write(2).
 *
 * readv(2) and writev(2) perform scather-gather IO on UNIX systems. They allow for
 * reading or writing multiple buffers to a file, while guarateeing atomicity, i.e.,
 * the bytes written or read will be consecutive.
 *
 * Usage:
 *
 *    $ ./readv_writev
 *    # Print performed steps to ensure implementation is correct.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE /* mkstemp function */

#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <malloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_SIZE 13

static char tmpFilePath[] = "/tmp/readv_writevXXXXXX";

void pexit(const char *fCall);
void fatal(const char *mesg);

ssize_t _readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t _writev(int fd, const struct iovec *iov, int iovcnt);

int
main() {
  int fd;
  struct iovec iovInput[3];
  struct iovec iovOutput[3];

  char code = 'x';                     /* First buffer  */
  int n = 100;                         /* Second buffer */
  char str[STR_SIZE] = "readv writev"; /* Third buffer  */

  char codeOutput;
  int nOutput;
  char strOutput[STR_SIZE];

  ssize_t numRead, numWritten, required = 0;

  fd = mkstemp(tmpFilePath);
  if (fd == -1) {
    pexit("open");
  }

  printf("Created file %s for scather-gather I/O\n", tmpFilePath);
  unlink(tmpFilePath);

  /* Set up input */
  iovInput[0].iov_base = &code;
  iovInput[0].iov_len = sizeof(code);
  required += iovInput[0].iov_len;

  iovInput[1].iov_base = &n;
  iovInput[1].iov_len = sizeof(n);
  required += iovInput[1].iov_len;

  iovInput[2].iov_base = str;
  iovInput[2].iov_len = sizeof(str);
  required += iovInput[2].iov_len;

  numWritten = _writev(fd, iovInput, 3);
  if (numWritten == -1) {
    fatal("Something wrong happened @_writev.");
  }

  if (numWritten != required) {
    fatal("Should write same number of bytes as requested.");
  }

  /* Set up output */
  iovOutput[0].iov_base = &codeOutput;
  iovOutput[0].iov_len = sizeof(codeOutput);

  iovOutput[1].iov_base = &nOutput;
  iovOutput[1].iov_len = sizeof(nOutput);

  iovOutput[2].iov_base = strOutput;
  iovOutput[2].iov_len = sizeof(strOutput);

  lseek(fd, 0, SEEK_SET);
  numRead = _readv(fd, iovOutput, 3);
  if (numRead == -1) {
    fatal("Something wrong happened @_readv");
  }

  printf("\nScather-gather I/O finished. Read data: code = %c, n = %d and str = \"%s\"\n", codeOutput, nOutput, strOutput);

  if (close(fd) == -1) {
    pexit("close");
  }

  return EXIT_SUCCESS;
}

ssize_t
_readv(int fd, const struct iovec *iov, int iovcnt) {
  int i;
  size_t memSize;
  ssize_t numCopied, numRead;
  void *buf;

  /* Calculates all the space that will be required */
  memSize = 0;
  for (i = 0; i < iovcnt; ++i) {
    memSize += iov[i].iov_len;
  }

  buf = malloc(memSize);
  if (buf == NULL) {
    pexit("malloc");
  }

  /* Reads all the data from the file into the buffer */
  numRead = read(fd, buf, memSize);
  if (numRead == -1) {
    return numRead;
  }

  /* Copies read that to the iovec structure */
  numCopied = 0;
  for (i = 0; i < iovcnt; ++i) {
    memcpy(iov[i].iov_base, buf + numCopied, iov[i].iov_len);
    numCopied += iov[i].iov_len;
  }

  free(buf);

  return numRead;
}

ssize_t
_writev(int fd, const struct iovec *iov, int iovcnt) {
  int i;
  size_t memSize;
  ssize_t numCopied, numWritten;
  void *buf;

  /* Calculates all the space that will be required */
  memSize = 0;
  for (i = 0; i < iovcnt; ++i) {
    memSize += iov[i].iov_len;
  }

  buf = malloc(memSize);
  if (buf == NULL) {
    pexit("malloc");
  }

  /* Copies data to the buffer */
  numCopied = 0;
  for (i = 0; i < iovcnt; ++i) {
    memcpy(buf + numCopied, iov[i].iov_base, iov[i].iov_len);
    numCopied += iov[i].iov_len;
  }

  numWritten = write(fd, buf, memSize);
  free(buf);

  return numWritten;
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

void
fatal(const char *mesg) {
  fprintf(stderr, "%s\n", mesg);
  exit(EXIT_FAILURE);
}
