/* hcp.c - A cp(1) utility that also copies holes.
 *
 * UNIX allows I/O to be performed past the end of the file. So, if you change the
 * file offset to a position past SEEK_END, for instance, and write to the file,
 * there will be ``hole'' in it. The intent of this program is to also copý holes
 * when copying two files.
 *
 * Usage examples
 *
 *    $ ./hcp file newfile
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>

#ifndef BUF_SIZ
#define BUF_SIZ 1024
#endif

typedef enum { FALSE, TRUE } Bool;

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);
int safeOpen(const char *pathname, int flags, ...);
void safeClose(int fd);

int
main(int argc, char *argv[]) {
  if (argc != 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  int inputFd, outputFd, inputFlags, outputFlags;
  mode_t outputMode;
  char *inputFile, *outputFile;

  ssize_t numRead;
  char buf[BUF_SIZ];
  char zeroes[BUF_SIZ];

  inputFile = argv[1];
  outputFile = argv[2];

  inputFlags = O_RDONLY;
  outputFlags = O_WRONLY | O_CREAT | O_TRUNC;
  outputMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; /* rw-rw-rw- */

  inputFd = safeOpen(inputFile, inputFlags);
  outputFd = safeOpen(outputFile, outputFlags, outputMode);

  while ((numRead = read(inputFd, buf, BUF_SIZ)) > 0) {
    if (numRead == -1) {
      pexit("read");
    }

    if (memcmp(buf, zeroes, numRead) == 0) {
      /* hole found, do not copy over */
      lseek(outputFd, numRead, SEEK_CUR);

    } else {
      if (write(outputFd, buf, numRead) != numRead) {
        pexit("write");
      }
    }
  }

  safeClose(inputFd);
  safeClose(outputFd);

  return EXIT_SUCCESS;
}

void
helpAndLeave(const char *progname, int status) {
  fprintf(stderr, "Usage: %s <file> <newfile>\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

int
safeOpen(const char *pathname, int flags, ... /* mode_t mode */) {
  va_list list;
  va_start(list, flags);
  mode_t mode;
  int fd;

  if (flags & O_CREAT) {
    mode = va_arg(list, mode_t);
    fd = open(pathname, flags, mode);

  } else {
    fd = open(pathname, flags);
  }

  if (fd == -1) {
    pexit("open");
  }

  return fd;
}

void
safeClose(int fd) {
  if (close(fd) == -1) {
    pexit("close");
  }
}
