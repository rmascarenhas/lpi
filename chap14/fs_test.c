/* fs_test.c - perform write/delete operations on different filesystems.
 *
 * This program analyses the performance of creating and deleting multiple
 * small files on different file systems. By default the files are created
 * with random and sparse names, and then removed in alphabetical order.
 * The order can be enforced to be alphabetical on creation as well on compilation
 * time by defining FS_TEST_ALPHA_ORDER.
 *
 * Usage
 *
 *    $ ./fs_test <NF> <DIR>
 *    # No output generated. You can analyse the time taken on multiple calls.
 *
 *    - NF: the number of files to be created/deleted.
 *    - DIR: the directory in which the temporary files should be created.
 *
 * Running this program in a Linux machine, with ext4 filesystem:
 *
 *    $ uname -a
 *    Linux 3.2.0-4-amd64 #1 SMP Debian 3.2.57-3 x86_64 GNU/Linux
 *
 * The table below shows a summary of the collected results:
 *
 *    SAMPLE FILES     RANDOM CREATION ORDER       SEQUENTIAL CREATION ORDER
 *                        real  0m0.057s                real  0m0.052s
 *       1,000            user  0m0.004s                user  0m0.004s
 *                        sys   0m0.052s                sys   0m0.044s
 *
 *                        real  0m0.491s                real  0m0.429s
 *      10,000            user  0m0.012s                user  0m0.020s
 *                        sys   0m0.472s                sys   0m0.400s
 *
 *                        real  0m0.956s                real  0m0.975s
 *      20,000            user  0m0.036s                user  0m0.040s
 *                        sys   0m0.904s                sys   0m0.808s
 *
 * Comparing the same set of results on a tmpfs on the same machine:
 *
 *    # mount -t tmpfs testfs testdir
 *    $ time ./fs_test <NF> testdir
 *
 * Results:
 *
 *    SAMPLE FILES     RANDOM CREATION ORDER       SEQUENTIAL CREATION ORDER
 *                        real  0m0.024s                real  0m0.023s
 *       1,000            user  0m0.004s                user  0m0.000s
 *                        sys   0m0.020s                sys   0m0.020s
 *
 *                        real  0m0.122s                real  0m0.113s
 *      10,000            user  0m0.024s                user  0m0.024s
 *                        sys   0m0.096s                sys   0m0.084s
 *
 *                        real  0m0.257s                real  0m0.218s
 *      20,000            user  0m0.052s                user  0m0.012s
 *                        sys   0m0.192s                sys   0m0.196s
 *
 * As can be seen from the results above, increasing the number of created/removed
 * files causes a proportional (probably linear) increase in the time necessary for the
 * program to finish. The tmpfs, however, is much faster than ext4, as expected:
 * it does not persist any data to the disk, avoiding any kind of latency that traditional
 * filesystems have to deal with.
 *
 * Additionaly, after a number of runs, it is also visible that creating and removing
 * files in order generates a small performance boost on ext4. This is probably because,
 * when files are removed in the order they were created, there is less time spent
 * seeking the next block to be removed: they are all close to each other. These sort
 * of issues are not present in tmpfs, so the order have no impact on it.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FS_TEST_BYTE ("w") /* one char is always one word long */

static int intComparison(const void *a, const void *b);

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
#define usageErr() \
  (helpAndLeave(argv[0], EXIT_FAILURE))

  char *location, *p;
  int nSamples, i;

  if (argc != 3) {
    usageErr();
  }

  nSamples = strtol(argv[1], &p, 10);
  if (p == argv[1]) {
    usageErr();
  }

  location = argv[2];

  if (chdir(location) == -1) {
    pexit("chdir");
  }

  char buf[BUFSIZ];
  int fd, filenameStamps[nSamples];

  srand(time(NULL));
  for (i = 0; i < nSamples; ++i) {
#ifdef FS_TEST_ALPHA_ORDER
    filenameStamps[i] = i;
#else
    filenameStamps[i] = rand() % 900000000 + 100000000;
#endif

    snprintf(buf, BUFSIZ, "x%06d", filenameStamps[i]);
    if ((fd = open(buf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1) {
      pexit("open");
    }

    if (write(fd, FS_TEST_BYTE, 1) < 1) {
      pexit("write");
    }

    if (close(fd) == -1) {
      pexit("close");
    }
  }

  qsort(filenameStamps, nSamples, sizeof(int), intComparison);

  for (i = 0; i < nSamples; ++i) {
    snprintf(buf, BUFSIZ, "x%06d", filenameStamps[i]);

    if (unlink(buf) == -1) {
      pexit("unlink");
    }
  }

  return EXIT_SUCCESS;
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <NF> <DIR>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

static int
intComparison(const void *a, const void *b) {
  return *(int *)a - *(int *)b;
}
