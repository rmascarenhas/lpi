/* dlog.c - logs events that occur on a directory and its descendants.
 *
 * This program is used to log all file events that occur in a directory and
 * its subtree. Events monitored include file creation, modification, removal,
 * changes of permission, and others.
 *
 * Usage
 *
 *    $ ./dlog <dir>
 *    # Logs information indefinitely
 *
 *    <dir> - the directory to be monitored.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE 500

#include <limits.h>
#ifndef NAME_MAX
#  include <linux/limits.h>
#endif

#include <ftw.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DLOG_NOPENFD
#  define DLOG_NOPENFD (100)
#endif

/* buffer size enough to allocate at least 10 events at once */
#ifndef DLOG_BUFSIZ
#  define DLOG_BUFSIZ (10 * (sizeof (struct inotify_event) + NAME_MAX + 1))
#endif

#ifndef DLOG_MAX_SUBDIRS
#  define DLOG_MAX_SUBDIRS (128)
#endif

#define Fatal(...) { \
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
}

/* structure to represent a watched subdirectory, child (at some level) of the
 * subtree root given as command line argument. This is useful so that the relative
 * path can be printed when logging events. Caching these paths allows us to save
 * system calls */
struct st_watched_subdir {
  int wd;
  char fpath[PATH_MAX];
};

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

/* the inotify descriptor needs to be public since it depends on the given
 * command line argument and needs to be known by the nftw traverse function */
int inotifyFd;

/* list of watched subdirs and control variable to indicate how many elements the
 * list currently holds. They are global since they can be modified by the
 * nftw iterator and when reading inotify events as well */
struct st_watched_subdir *watchedDirs[DLOG_MAX_SUBDIRS];
int watchedDirsCount = 0;

/* the function that will be passed to nftw in order to install a monitor in every
 * directory in the subtree */
static int installMonitor(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

/* displays information about the passed event that was read */
static void logEvent(struct inotify_event *event);

static void addWatch(const char *path);
static void rmWatch(int wd);

int
main(int argc, char *argv[]) {
  ssize_t numRead;
  char *p, buf[DLOG_BUFSIZ];
  struct inotify_event *event;

  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  inotifyFd = inotify_init();
  if (inotifyFd == -1) {
    pexit("inotify_init");
  }

  /* traverse the directory tree and monitor all directories */
  if (nftw(argv[1], installMonitor, DLOG_NOPENFD, FTW_PHYS) == -1) {
    pexit("nftw");
  }

  /* read incoming events indefinitely */
  printf("Listening for events on %s...\n", argv[1]);
  for (;;) {
    numRead = read(inotifyFd, buf, DLOG_BUFSIZ);
    if (numRead == -1) {
      pexit("read");
    }

    for (p = buf; p < buf + numRead; ) {
      event = (struct inotify_event *) p;
      logEvent(event);

      p += sizeof(struct inotify_event) + event->len;
    }
  }

  exit(EXIT_SUCCESS);
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <dir>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

static void
addWatch(const char *path) {
  int wd;
  struct st_watched_subdir *entry;

  if (watchedDirsCount >= DLOG_MAX_SUBDIRS) {
    Fatal("Max watched dir limit reached: %d\n", DLOG_MAX_SUBDIRS);
  }

  wd = inotify_add_watch(inotifyFd, path, IN_ALL_EVENTS);
  if (wd == -1) {
    pexit("inotify_add_watch");
  }

  entry = malloc(sizeof(struct st_watched_subdir));
  if (entry == NULL) {
    pexit("malloc");
  }

  entry->wd = wd;
  strncpy(entry->fpath, path, PATH_MAX);

  watchedDirs[watchedDirsCount++] = entry;
}

static void
rmWatch(int wd) {
  int i;

  for (i = 0; i < watchedDirsCount; ++i) {
    if (watchedDirs[i]->wd == wd) {
      free(watchedDirs[i]);
      watchedDirs[i] = NULL;

      return;
    }
  }
}

static int
installMonitor(const char *fpath, const struct stat *sb, int typeflag, __attribute__((unused)) struct FTW *ftwbuf) {
  /* file could not be read, possibly by lack of permissions: skip it and move on */
  if (typeflag == FTW_NS || typeflag == FTW_DNR) {
    return 0;
  }

  /* add a watch only to directories */
  if (S_ISDIR(sb->st_mode)) {
    addWatch(fpath);
  }

  return 0;
}

char *
pathPrefix(int wd) {
  int i;

  for (i = 0; i < watchedDirsCount; ++i) {
    if (watchedDirs[i]->wd == wd) {
      return watchedDirs[i]->fpath;
    }
  }

  Fatal("Could not find path prefix for wd %d\n", wd);
}

static void
logEvent(struct inotify_event *event) {
#define Dlog(label, msg) { \
  printf("[%s] ", label); \
  if (event->len > 0) { \
    if (event->mask & IN_ISDIR) { \
      printf("Directory %s/%s %s\n", pathPrefix(event->wd), event->name, msg); \
    } else { \
      printf("File %s/%s %s\n", pathPrefix(event->wd), event->name, msg); \
    } \
  } else { \
    printf("Directory %s %s\n", pathPrefix(event->wd), msg); \
  } \
}

  if (event->mask & IN_ACCESS)        Dlog("INFO", "was accessed");
  if (event->mask & IN_ATTRIB)        Dlog("INFO", "had its metadata changed");
  if (event->mask & IN_CLOSE_NOWRITE) Dlog("INFO", "was closed (read-only)");
  if (event->mask & IN_CLOSE_WRITE)   Dlog("INFO", "was closed");
  if (event->mask & IN_CREATE)        Dlog("INFO", "was created");
  if (event->mask & IN_DELETE)        Dlog("INFO", "was deleted");
  if (event->mask & IN_DELETE_SELF)   Dlog("WARNING", "was deleted (watched directory)");
  if (event->mask & IN_IGNORED)       Dlog("WARNING", "is no longer being watched (maybe it was deleted?)");
  if (event->mask & IN_MODIFY)        Dlog("INFO", "was modified");
  if (event->mask & IN_MOVE_SELF)     Dlog("INFO", "was moved");
  if (event->mask & IN_MOVED_FROM)    Dlog("INFO", "was moved");
  if (event->mask & IN_MOVED_TO)      Dlog("INFO", "was moved");
  if (event->mask & IN_OPEN)          Dlog("INFO", "was opened");
  if (event->mask & IN_Q_OVERFLOW)    Dlog("FATAL", "too many queued file events");
  if (event->mask & IN_UNMOUNT)       Dlog("INFO", "was unmounted");

  if (event->mask & IN_IGNORED) {
    rmWatch(event->wd);
  }

  if (event->mask & IN_ISDIR && event->mask & IN_CREATE) {
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/%s", pathPrefix(event->wd), event->name);

    addWatch(path);
  }
}
