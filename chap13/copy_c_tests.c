/* copy_c_tests.c - a simple copy program.
 *
 * This is an adaptation of Listing 4-1 of The Linux Programming Interface book,
 * run with different buffer sizes and on different file systems.
 *
 * It is possible to overwrite the size of the buffer used to read the file by
 * defining BUF_SIZE as in:
 *
 *    $ c99 -DBUF_SIZE 256 copy_c_tests.c
 *
 * To use the `O_SYNC` flag and do not use the kernel's buffers, define SYNC_WRITE
 * when compiling the file:
 *
 *    $ c99 -DSYNC_WRITE copy_c_tests.c
 *
 * The default buffer size is 1024 bytes.
 *
 * This program was run in different environments to test the performance with
 * different buffer sizes and different filesystems.
 *
 * Results
 * =======
 *
 * ## Varying the buffer size.
 *
 * The tests were performed on a Linux machine, copying a 20M PDF file,
 * using an ext4 filesystem:
 *
 *    $ uname -a
 *    Linux 3.2.0-4-amd64 #1 SMP Debian 3.2.57-3 x86_64 GNU/Linux
 *
 * BUFFER SIZE: 1B             WITH THE O_SYNC FLAG
 *    real  0m48.766s
 *    user  0m2.016s          Just too big to measure
 *    sys   0m46.559s
 *
 * BUFFER SIZE: 32B
 *    real  0m1.585s
 *    user  0m0.052s          Just too big to measure
 *    sys   0m1.528s
 *
 * BUFFER SIZE: 128B
 *    real  0m0.417s
 *    user  0m0.016s          Just too big to measure
 *    sys   0m0.396s
 *
 * BUFFER SIZE: 1024B       WITH THE O_SYNC FLAG
 *    real  0m0.073s            real  15m47.106s
 *    user  0m0.000s            user  0m0.056s
 *    sys   0m0.072s            sys   0m2.980s
 *
 * BUFFER SIZE: 4096B       WITH THE O_SYNC FLAG
 *    real  0m0.071s            real  4m23.828s
 *    user  0m0.000s            user  0m0.004s
 *    sys   0m0.048s            sys   0m0.956s
 *
 * As we can see from the results above, the size of the buffers used do affect
 * the overall performance of the copy program. However, when a certain threshold
 * is reached, little is gained from larger buffers, since writes to the kernel
 * buffers are fast.
 *
 * When we add the O_SYNC flag, we completely skip the kernel cache and force that
 * every write system call actually saves the data to the disk. This has a **huge**
 * impact as can be seen in the results. For smaller buffer sizes, the time is just
 * too big to even consider (I left it copying overnight and it was not enough
 * for a whole copy).
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef BUF_SIZE
#  define BUF_SIZE 1024
#endif

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  int inputFd, outputFd, openFlags;
  mode_t filePerms;
  ssize_t numRead;
  char buf[BUF_SIZE];

  if (argc != 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  /* open input and output files */

  inputFd = open(argv[1], O_RDONLY);
  if (inputFd == -1) {
    pexit("open");
  }

  openFlags = O_CREAT | O_WRONLY | O_TRUNC;
#ifdef SYNC_WRITE
  openFlags |= O_SYNC;
#endif

  filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; /* rw-rw-rw */
  outputFd = open(argv[2], openFlags, filePerms);

  if (outputFd == -1) {
    pexit("open");
  }

  /* transfer data until we encounter an end of input or an error */

  while ((numRead = read(inputFd, buf, BUF_SIZE)) > 0) {
    if (write(outputFd, buf, numRead) != numRead) {
      pexit("write");
    }
  }

  if (numRead == -1) {
    pexit("read");
  }

  if (close(inputFd) == -1) {
    pexit("close");
  }

  if (close(outputFd) == -1) {
    pexit("close");
  }

  return EXIT_SUCCESS;
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "%s <oldfile> <newfile>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
