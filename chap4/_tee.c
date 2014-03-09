/*
 * _tee.c - a minimum tee(1) implementation
 *
 * The tee(1) utility copies its standard input to both stdout and  to a file.
 * This allows the user to view the output of a command on the console while
 * writing a log to a file at the same time.
 *
 * Usage examples
 *
 *    $ echo ZOMG | tee file.txt # outputs the string ZOMG and writes it to file.txt
 *
 * By dfefault, tee will overwrite the contents of the passed file. Passing the -a option
 * tells _tee to append to the file instead.
 *
 * Supported command line options:
 *
 *    -a    Append to the file instead of overwriting it
 *    -h    Displays help message and exit
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BUF_SIZ
#define BUF_SIZ 1024
#endif

typedef enum { FALSE, TRUE } Bool;

void helpAndLeave(const char *progname, int status);
void failure(const char *fCall);

int
main(int argc, char *argv[]) {
  int opt;
  Bool append = FALSE;
  char *filename;

  int fd, flags;
  mode_t mode;
  char buf[BUF_SIZ + 1];
  ssize_t numRead;

  /* Command line arguments parsing */

  opterr = 0;
  while ((opt = getopt(argc, argv, "+a")) != -1) {
    switch(opt) {
      case '?': helpAndLeave(argv[0], EXIT_FAILURE); break;
      case 'h': helpAndLeave(argv[0], EXIT_SUCCESS); break;
      case 'a': append = TRUE;                       break;
    }
  }

  if (optind >= argc) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  filename = argv[optind];

  /* stdin redirection */

  flags = O_CREAT | O_WRONLY;
  mode = S_IRUSR | S_IWUSR; /* rw------- */

  if (append) {
    flags |= O_APPEND;
  } else {
    flags |= O_TRUNC;
  }

  fd = open(filename, flags, mode);
  if (fd == -1) {
    failure("open");
  }

  while ((numRead = read(STDIN_FILENO, buf, BUF_SIZ)) > 0) {
    if (numRead == -1) {
      failure("read");
    }

    if (write(STDOUT_FILENO, buf, numRead) != numRead) {
      failure("write");
    }

    if (write(fd, buf, numRead) != numRead) {
      failure("write");
    }
  }

  if (close(fd) == -1) {
    failure("close");
  }

  return 0;
}

void
helpAndLeave(const char *progname, int status) {
  fprintf(stderr, "Usage: %s [-a] <file>\n", progname);
  exit(status);
}

void
failure(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
