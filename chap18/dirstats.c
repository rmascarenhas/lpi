/* dir_stats.c - display file type stats of a directory.
 *
 * This program takes a directory as argument (or uses the current directory if
 * none is given) to display some statistics of the file types over the directory
 * hierarchy. The amount of files - and its percentage compared to the total - are
 * displayed for various file types.
 *
 * This program illustrates the usage of the `nftw(3)` library function to traverse
 * a directory tree.
 *
 * Usage
 *
 *    $ ./dirstats [-n] [<directory>]
 *
 *    -n - do not follow symbolic links (default is to follow)
 *
 *    directory - the directory to traverse. If none is given, the current working
 *    directory is taken.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 500

#include <ftw.h>
#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DIRSTATS_NOPENFD
#  define DIRSTATS_NOPENFD (4096)
#endif

typedef enum { FALSE, TRUE } Bool;

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static int getStats(const char *dir, const int flags);
static void printStats(const char *dir);

struct file_count {
  size_t reg, dir, chr, blk, fifo, lnk, sock;
  size_t unreadDir, unreadFile;
};

/* needs to be global so that changes on the `nftw` function are visible to the
 * outside world */
struct file_count fstats = {
  .reg  = 0,
  .dir  = 0,
  .chr  = 0,
  .blk  = 0,
  .fifo = 0,
  .lnk  = 0,
  .sock = 0
};

int
main(int argc, char *argv[]) {
  if (argc > 3) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  char *dir;
  int flags, opt;

  flags = 0;
  while ((opt = getopt(argc, argv, "n")) != -1) {
    switch (opt) {
      case 'n': flags |= FTW_PHYS;                   break;
      default:  helpAndLeave(argv[0], EXIT_FAILURE); break;
    }
  }

  if (argv[optind]) {
    dir = argv[optind];
  } else {
    dir = ".";
  }

  printf("Scanning files...\n");
  if (getStats(dir, flags) == -1) {
    pexit("getStats");
  }

  printStats(dir);

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s [-n] [<directory>]\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

/* function to be called by `nftw(3)` */
static int
analyzeFile(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) {
  if (type == FTW_DNR) {
    /* directory that could not be read */
    ++fstats.unreadDir;
    return 0;
  }

  if (type == FTW_NS) {
    /* stat failed - the stat buffer content is undefined */
    ++fstats.unreadFile;
    return 0;
  }

  switch (sbuf->st_mode & S_IFMT) {
    case S_IFREG:
      ++fstats.reg;
      break;

    case S_IFDIR:
      ++fstats.dir;
      break;

    case S_IFCHR:
      ++fstats.chr;
      break;

    case S_IFBLK:
      ++fstats.blk;
      break;

    case S_IFLNK:
      ++fstats.lnk;
      break;

    case S_IFIFO:
      ++fstats.fifo;
      break;

    case S_IFSOCK:
      ++fstats.sock;
      break;

    default:
      printf("unrecognizable file: %s (level %d)\n", pathname, ftwb->level);
      return -1;
  }

  return 0;
}

static int
getStats(const char *dir, const int flags) {
  if (nftw(dir, analyzeFile, DIRSTATS_NOPENFD, flags) == -1) {
    return -1;
  }

  return 0;
}

static void
printStat(const char *ftype, size_t num, size_t total) {
  if (num > 0) {
    double percent;

    percent = ((double)num / total) * 100;
    printf("\t%20s: %10ld (%5.5lf%%)\n", ftype, (long) num, percent);
  }
}

static void
printStats(const char *dir) {
  size_t total;

  total = fstats.reg + fstats.dir + fstats.chr + fstats.blk + fstats.fifo + fstats.lnk + fstats.sock;

  printf("\n");
  printf("File statistics for %s:\n", dir);

  printStat("Regular files",     fstats.reg,  total);
  printStat("Directories",       fstats.dir,  total);
  printStat("Character devices", fstats.chr,  total);
  printStat("Block devices",     fstats.blk,  total);
  printStat("FIFOs",             fstats.fifo, total);
  printStat("Symbolic links",    fstats.lnk,  total);
  printStat("Sockets",           fstats.sock, total);
  printf("\t===========================================\n");
  printf("%20s: %10s %7ld\n", "Total", "", (long) total);

  printf("\nFinished. %ld unread directories and %ld unread files\n",
    (long) fstats.unreadDir, (long) fstats.unreadFile);
}
