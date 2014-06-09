/* tail.c - a simple implementation of the tail(1) utility.
 *
 * This program is a simple implementation of the `tail(1)` utility, which prints
 * the last lines of a file. The only command line switch this program supports
 * is -n, which tells it how many lines to print. If it is not given, the last
 * ten lines are print.
 *
 * Usage
 *
 *    $ ./tail [-n NUM] <file>
 *    Line 1
 *    Line 2
 *    Line 3
 *    ...
 *
 *    -n: tells the program to print the last NUM lines
 *    <file> - the file to be print
 *
 * Note that this program do not support arbitrarily long strings. However,
 * it should produce an output in constant time regarding the input size and
 * linear time according to the number of lines to be print.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 600 /* getopt and posix_fadivse */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TAIL_BUFSIZ
#  define TAIL_BUFSIZ BUFSIZ
#endif

#define Min(a, b) ((a) < (b) ? (a) : (b))

typedef enum { FALSE, TRUE } Bool;

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  if (argc != 2 && argc != 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  int opt;
  int count = 10;
  char *file, *p;

  opterr = 0;
  while ((opt = getopt(argc, argv, "n:")) != -1) {
    switch (opt) {
      case 'n':
        errno = 0;
        count = strtol(optarg, &p, 10);
        if (errno != 0) {
          pexit("strtol");
        }

        /* given argument is not a valid number */
        if (p == optarg) {
          helpAndLeave(argv[0], EXIT_FAILURE);
        }

        break;

      case '?':
        helpAndLeave(argv[0], EXIT_FAILURE);
        break;
    }
  }

  file = argv[optind];
  if (file == NULL) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  int fd, numLines, i, textSize;
  char buf[TAIL_BUFSIZ + 1], text[count * BUFSIZ];
  off_t fileSize, offCur;
  ssize_t numRead;
  Bool done;

  fd = open(file, O_RDONLY);
  if (fd == -1) {
    pexit("open");
  }

  if ((fileSize = lseek(fd, 0, SEEK_END)) == -1) {
    pexit("lseek");
  }

  /* we are going to read the file backwards, so we continuouly position the file
   * cursor a number of bytes before its end */
  if ((offCur = lseek(fd, -1 * Min(TAIL_BUFSIZ, fileSize), SEEK_CUR)) == -1) {
    pexit("lseek");
  }

  numLines = textSize = 0;
  done = FALSE;
  while (!done && (numRead = read(fd, buf, TAIL_BUFSIZ)) != 0) {
    /* tell the kernel we are going to need to next bytes backwards in
     * the file soon */
    posix_fadvise(fd, offCur - TAIL_BUFSIZ, TAIL_BUFSIZ, POSIX_FADV_WILLNEED);

    buf[numRead] = '\0';
    for (i = numRead - 1; !done && i >= 0; --i) {
      if (buf[i] == '\n') {
        numLines++;
      }

      if (numLines > count) {
        done = TRUE;
      } else {
        text[textSize++] = buf[i];
      }
    }

    if (!done) {
      if ((offCur = lseek(fd, -2 * Min(TAIL_BUFSIZ, offCur), SEEK_CUR)) == -1) {
        pexit("lseek");
      }
    }
  }

  if (numRead == -1) {
    pexit("read");
  }

  if (close(fd) == -1) {
    pexit("close");
  }

  for (i = textSize -1; i >= 0; --i) {
    printf("%c", text[i]);
  }

  return EXIT_SUCCESS;
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s [-n NUM] <file>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
