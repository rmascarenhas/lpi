/* running.c - Displays process IDs and command lines for processes a given user is running.
 *
 * This program takes a user as an argument and prints all processes of that user
 * that are currently running. Both the process ID and the command line used to start
 * the process are shown.
 *
 * Usage
 *
 *    $ ./running [<username>]
 *      PID             COMMAND
 *    28418             running
 *     2864     gnome-keyring-d
 *     2940             openbox
 *     2986           ssh-agent
 *     2989         dbus-launch
 *     2990         dbus-daemon
 *     3000               tint2
 *
 *    <username> - the username to be searched. If omitted, the current user
 *    (owner of the parent process) is used.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
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

uid_t uidFromUsername(char *username);

int
main(int argc, char *argv[]) {
  int status;
  uid_t uid;

  if (argc == 1) {
    uid = geteuid();
  } else if (argc == 2) {
    status = uidFromUsername(argv[1]);

    if (status == -1) {
      fprintf(stderr, "%s: Username not found: %s\n", argv[0], argv[1]);
      exit(EXIT_FAILURE);
    } else {
      uid = status;
    }
  } else {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  struct dirent *procpid;
  int statusFd;
  ssize_t numRead;
  long pid, processCount;
  char statusFile[BUF_SIZ], matching[BUF_SIZ], command[BUF_SIZ], buf[STATUS_FILE_MAX];
  char *line;
  Bool done, ownerFound, pidFound, commandFound;

  DIR *proc = opendir(PROC_FS);
  if (proc == NULL) {
    pexit("opendir");
  }

  printf("Process information for user ID #%ld\n\n", (long) uid);
  processCount = 0;

  printf("%8s%20s\n", "PID", "COMMAND");
  for (errno = 0; (procpid = readdir(proc)) != NULL; errno = 0) {
    snprintf(statusFile, BUF_SIZ, "%s/%s/status", PROC_FS, procpid->d_name);
    statusFd = open(statusFile, O_RDONLY);

    if (statusFd == -1) {
      /* the directory do not contain a status file */
      if (errno != ENOENT && errno != ENOTDIR) {
        pexit("open");
      }

      continue;
    }

    /* read the whole file in memory to make sure field comparisons will work */
    numRead = read(statusFd, buf, STATUS_FILE_MAX);
    if (numRead == -1) {
      pexit("read");
    } else if (numRead == STATUS_FILE_MAX) {
      fprintf(stderr, "Your status file seems to be too big (> 10kB). Nothing we can do.\n");
      exit(EXIT_FAILURE);
    }

    if (close(statusFd)) {
      pexit("close");
    }

    done  = FALSE; /* are we done processing the status file? */
    ownerFound = pidFound = commandFound = FALSE;
    while (!done) {
      line = strtok(buf, "\n");

      pid = -1;
      while (line != NULL) {
        /* check for the user of the process */
        if (strncmp(line, "Uid:", 4) == 0) {
          snprintf(matching, BUF_SIZ, "Uid:\t%ld", (long) uid);
          if (strncmp(line, matching, strlen(matching)) == 0) {
            ownerFound = TRUE;
          } else {
            /* this process is not from the user we want, we can stop processing */
            done = TRUE;
          }
        }

        /* check for Pid */
        if (strncmp(line, "Pid:", 4) == 0) {
          pid = atol(line + 4);
          pidFound = TRUE;
        }

        /* check for command name */
        if (strncmp(line, "Name:", 5) == 0) {
          strncpy(command, line + 6, BUF_SIZ);
          commandFound = TRUE;
        }

        line = strtok(NULL, "\n");
        done = done || (ownerFound && pidFound && commandFound);
      }
    }

    /* process if from user being searched */
    if (ownerFound) {
      ++processCount;
      printf("%8ld%20s\n", pid, command);
    }
  }

  if (closedir(proc) == -1) {
    pexit("closedir");
  }

  printf("%ld processes found.\n", processCount);
  return EXIT_SUCCESS;
}

void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "%s [<username>]\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

uid_t
uidFromUsername(char *username) {
  struct passwd *user;

  errno = 0;
  user = getpwnam(username);

  if (errno == 0) {
    if (user == NULL) {
      /* username was not fond */
      return -1;
    } else {
      return user->pw_uid;
    }
  } else {
    /* error in getpwnam(3) call */
    pexit("getpwnam");
    return -1; /* should never get to this point */
  }
}
