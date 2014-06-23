/* nftw.c - an implementation of the `nftw(3)` library function.
 *
 * This implements the `nftw(3)` function using lower level functions to traverse
 * a directory hierarchy.
 *
 * The behavior of this program is the same of the `dirstats` one present in this
 * same directory. The only difference is that the custom `nftw` function is used
 * here. Refer to that file for more info and usage examples.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 500
#define _BSD_SOURCE

#include <limits.h>

#ifndef NAME_MAX
#  include <linux/limits.h>
#endif

#include <ftw.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifndef DIRSTATS_NOPENFD
#  define DIRSTATS_NOPENFD (100)
#endif

typedef enum { FALSE, TRUE } Bool;

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

static int getStats(const char *dir, const int flags);
static void printStats(const char *dir);

static int _nftw(const char *dirpath,
    int (*fn) (const char *fpath, const struct stat *sb,
               int typeflag, struct FTW *ftwbuf),
    int nopenfd, int flags);

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
  if (_nftw(dir, analyzeFile, DIRSTATS_NOPENFD, flags) == -1) {
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

#ifdef _DIRENT_HAVE_D_TYPE
static int
failedStatType(const struct dirent *dir) {
  if (dir->d_type == DT_DIR) {
    return FTW_DNR;
  } else {
    return FTW_NS;
  }
}
#else
static int
failedStatType(const struct dirent *sBuf) {
  return FTW_NS;
}
#endif

static int
_nftwRec(const char *dirpath,
    int (*fn) (const char *fpath, const struct stat *sb,
               int typeflag, struct FTW *ftwbuf),
    int nopenfd, int flags, int base, int level, int *fdCount, dev_t devid, struct stat *sCwdBuf) {

  DIR *dirp;
  struct dirent dir, *result;
  int type, status, readNum, i;
  char fpath[PATH_MAX];

  struct stat sBuf;
  struct FTW ftwbuf;

  Bool ftwns, depth, closed;

  type = FTW_D;

  errno = 0;
  dirp = opendir(dirpath);
  if (dirp == NULL) {
    if (errno == EACCES) {
      type = FTW_DNR;
    } else {
      return -1;
    }
  }

  ++(*fdCount);

  /* chdir to the directory if specified on the flags parameter */
  if (flags & FTW_CHDIR) {
    if (chdir(dirpath) == -1) {
      return -1;
    }
  }

  ftwbuf.level = level;
  ftwbuf.base = base;

  depth = (flags & FTW_DEPTH);

  if (!depth) {
    status = fn(dirpath, sCwdBuf, type, &ftwbuf);

    if (status != 0) {
      return status;
    }

    if (type == FTW_DNR) {
      return status;
    }
  }

  readNum = 0;
  while (readdir_r(dirp, &dir, &result) == 0 && result != NULL) {
    ++readNum;
    ftwns = FALSE;
    closed = FALSE;
    ftwbuf.base = base + strlen(dir.d_name) + 1; /* account for the slash character */

    /* do not handle . and .. entries */
    if (strncmp(dir.d_name, ".", NAME_MAX) && strncmp(dir.d_name, "..", NAME_MAX)) {
      snprintf(fpath, PATH_MAX, "%s/%s", dirpath, dir.d_name);

      if (flags & FTW_PHYS) {
        /* do not follow symbolic links */
        if (lstat(fpath, &sBuf) == -1) {
          type = failedStatType(&dir);
          ftwns = TRUE;
        }
      } else {
        /* dereference symbolic links */
        if (stat(fpath, &sBuf) == -1) {
          type = failedStatType(&dir);
          ftwns = TRUE;
        }
      }

      /* do not cross mount points if FTW_MOUNT was passed */
      if (!ftwns && (flags & FTW_MOUNT) && sBuf.st_dev != devid) {
        continue;
      }

      if (ftwns) {
        fn(fpath, NULL, type, &ftwbuf);
      } else {
        if (S_ISDIR(sBuf.st_mode)) {
          /* maximum allowed number of open files reached. Close currrent
           * directory and restore it after recursive call */
          if (*fdCount >= nopenfd) {
            if (closedir(dirp) == -1) {
              return -1;
            }
            closed = TRUE;
            --(*fdCount);
          }

          status = _nftwRec(fpath, fn, nopenfd, flags, ftwbuf.base, ++level, fdCount, devid, &sBuf);
          --(*fdCount);

          if (status != 0) {
            return status;
          }

          if (closed) {
            dirp = opendir(dirpath);
            if (dirp == NULL) {
              return -1;
            }

            ++(*fdCount);

            /* restore the directory currently being iterated */
            for (i = 0; i < readNum; ++i) {
              readdir_r(dirp, &dir, &result);
            }
          }
        } else {
          if (S_ISLNK(sBuf.st_mode)) {
            type = FTW_SL;
          } else {
            type = FTW_F;
          }

          status = fn(fpath, &sBuf, type, &ftwbuf);

          if (status != 0) {
            return status;
          }
        }
      }
    }
  }

  /* all entries of the current directory were processed - process the current
   * directory if it was not already processed */
  if (depth) {
    type = FTW_DP;
    ftwbuf.base = base;

    fn(dirpath, sCwdBuf, type, &ftwbuf);
  }

  if (closedir(dirp) == -1) {
    return -1;
  }

  return status;
}


/* the nftw function - wrap the recursive function and makes sure the current working
 * directory is unchanged after the call */
static int
_nftw(const char *dirpath,
    int (*fn) (const char *fpath, const struct stat *sb,
               int typeflag, struct FTW *ftwbuf),
    int nopenfd, int flags) {

  int cwdfd, status;
  struct stat sBuf;

  cwdfd = open(".", O_RDONLY);
  if (cwdfd == -1) {
    return -1;
  }

  if (fstat(cwdfd, &sBuf) == -1) {
    return -1;
  }

  int fdCount = 0;
  status = _nftwRec(dirpath, fn, nopenfd, flags, 0, 0, &fdCount, sBuf.st_dev, &sBuf);

  if (fchdir(cwdfd) == -1) {
    return -1;
  }

  return status;
}
