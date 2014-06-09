/* pf.c - List processes that opened a certain file.
 *
 * This program takes a filename as its argument and then inspects the /proc
 * filesystem in order to find all the processes that opened that file.
 * The result is a list of a PID/command of the processes that have that
 * file open.
 *
 * Usage
 *
 *    $ sudo ./pf <filename>
 *     PID   FD             COMMAND
 *    2703    4               vim
 *    4491    8               nano
 *    6645    3               more
 *
 *    <filename> - the name of the file that is going to be considered.
 *
 * Note that the filename argument must be given as an absolute path to the
 * destination, since the links are stored this way.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _BSD_SOURCE /* readlink function */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROC_FS ("/proc")
#define BUF_SIZ (512)
#define STATUS_FILE_MAX (10240)

typedef enum { FALSE, TRUE } Bool;

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);

/* Internal: prints formatted PID and command for a given process. The statusFd argument
 * must be a descriptor of the status file of the corresponding process, and the fd
 * argument represents the fd currently being verified */
void printProcessInfo(int statusFd, char *fd, char *pid);

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  char *filename = argv[1];
  char pathname[BUF_SIZ], buf[BUF_SIZ];
  int statusFd;
  ssize_t numRead;

  DIR *proc, *procpid;                    /* /proc and /proc/PID/fd, respectively */
  struct dirent *procpid_d, *procpidfd_d; /* /proc/PID/status and /proc/PID/fd/FD, respectively */

  proc = opendir(PROC_FS);
  if (proc == NULL) {
    pexit("opendir");
  }

  printf("%8s%5s%20s\n", "PID", "FD", "COMMAND");
  for (errno = 0; (procpid_d = readdir(proc)) != NULL; errno = 0) {
    /* skip /proc/self link */
    if (strncmp(procpid_d->d_name, "self", 4) == 0) {
      continue;
    }

    snprintf(buf, BUF_SIZ, "%s/%s/status", PROC_FS, procpid_d->d_name);
    statusFd = open(buf, O_RDONLY);

    if (statusFd == -1) {
      if (errno != ENOENT && errno != ENOTDIR) {
        pexit("open");
      }

      /* skip directories under /proc that do not have a status file - that is
       * our logic to identify process directories under the proc filesystem */
      continue;
    }

    /* /proc/PID/fd directory */
    snprintf(buf, BUF_SIZ, "%s/%s/fd", PROC_FS, procpid_d->d_name);

    procpid = opendir(buf);
    if (procpid == NULL) {
      pexit("opendir");
    }

    /* loop through every file descriptor open for a process */
    for (errno = 0; (procpidfd_d = readdir(procpid)) != NULL; errno = 0) {
      /* skip . and .. entries */
      if (strncmp(procpidfd_d->d_name, ".", 1) == 0 || strncmp(procpidfd_d->d_name, "..", 2) == 0) {
        continue;
      }

      /* build full path to the file descriptor link */
      snprintf(pathname, BUF_SIZ, "%s/%s/fd/%s", PROC_FS, procpid_d->d_name, procpidfd_d->d_name);

      numRead = readlink(pathname, buf, BUF_SIZ);
      if (numRead == -1) {
        pexit("readlink");
      }

      /* readlink(2) does not append the null byte at the end of the read data,
       * so we do it ourselves */
      buf[numRead] = '\0';

      if (strncmp(buf, filename, BUF_SIZ) == 0) {
        printProcessInfo(statusFd, procpidfd_d->d_name, procpid_d->d_name);
      }
    }

    /* left the loop with error */
    if (errno != 0) {
      pexit("readdir");
    }

    if (closedir(procpid) == -1) {
      pexit("closedir");
    }

    if (close(statusFd) == -1) {
      pexit("close");
    }
  }

  if (errno != 0) {
    pexit("readdir");
  }

  if (closedir(proc) == -1) {
    pexit("closedir");
  }

  return EXIT_SUCCESS;
}

void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <filename>\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

void
printProcessInfo(int statusFd, char *fd, char *pid) {
  char command[BUF_SIZ], statusFile[STATUS_FILE_MAX], *line;
  ssize_t numRead;
  Bool commandFound;

  numRead = read(statusFd, statusFile, STATUS_FILE_MAX);
  if (numRead == -1) {
    pexit("read");
  } else if (numRead == STATUS_FILE_MAX) {
    fprintf(stderr, "Your status file seems to be too big (> 10kB). Nothing we can do.\n");
    exit(EXIT_FAILURE);
  }

  line = strtok(statusFile, "\n");
  commandFound = FALSE;

  while (!commandFound && line != NULL) {
    /* check for the Name field */
    if (strncmp(line, "Name:", 5) == 0) {
      strncpy(command, line + 5, BUF_SIZ);
      commandFound = TRUE;
    }

    line = strtok(NULL, "\n");
  }

  if (commandFound) {
    printf("%8s%5s%20s\n", pid, fd, command);
  }
}
