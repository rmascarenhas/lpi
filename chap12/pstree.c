/* pstree.c - A simpler implementation of the pstree(1) command.
 *
 * This program analizes the processes in the system and builds a visual
 * representation of their hierarchy, printing a tree-like structure of
 * the processes and their parents.
 *
 * Usage
 *
 *    $ ./pstree
 *    - (1) init
 *      - (454) udevd
 *        - (18252) udevd
 *        - (18253) udevd
 *      - (2070) rsyslogd
 *      - (2191) ntpd
   *   # lots more...
 *
 * Author: Renato Mascarenhas Costa
 */

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
#define CHILDREN_MAX (30567)
#define STATUS_FILE_MAX (10240)

/* the root of the process hierarchy is the `init` process with PID 1 in
 * UNIX systems. */
#define INIT_PID (1)

typedef enum { FALSE, TRUE } Bool;

struct process {
  char command[BUF_SIZ];
  int children_count;
  uid_t children[CHILDREN_MAX];
};

void pexit(const char *fCall);

/* Reads the /proc filesystem to fetch the maximum number allowed for a PID
 * in the system */
long getPidMax();

/* Scans the /proc filesystem for each process and fills in the given array of
 * `process` structure, which must have been allocated beforehand. */
void buildProcessDataStructure(struct process *processes);

/* Prints a tree with the data containing in the array of processes passed as
 * argument. A call to `buildProcessDataStructure` must procede a call to this
 * function so that the process hierarchy is correctly calculated. The tree
 * is built from the `root` parameter passed. */
void printTree(struct process *processes, int root, int level);

int
main() {
  long pid_max;

  /* this is the data structure that will hold all the information of the whole
   * operating system process hierarchy. After the /proc filesystem is parsed,
   * this data structure is filled and will allow the program to correctly print
   * the process tree from the root */
  struct process *processes;

  /* get the maximum PID number for the current system */
  pid_max = getPidMax();
  if (pid_max == -1) {
    pexit("getPidMax");
  }

  processes = malloc(pid_max * sizeof(struct process));

  buildProcessDataStructure(processes);
  printTree(processes, INIT_PID, 0);

  free(processes);

  return EXIT_SUCCESS;
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

long
getPidMax() {
  char buf[BUF_SIZ];
  ssize_t numRead;
  int fd;
  long pid_max;

  snprintf(buf, BUF_SIZ, "%s/sys/kernel/pid_max", PROC_FS);
  fd = open(buf, O_RDONLY);
  if (fd == -1) {
    return -1;
  }

  numRead = read(fd, buf, BUF_SIZ);
  if (numRead == -1) {
    return -1;
  }

  pid_max = atol(buf);

  if (pid_max == 0) {
    /* could not convert to a number */
    return -1;
  }

  return pid_max;
}

void
buildProcessDataStructure(struct process *processes) {
  DIR *proc;
  struct dirent *procpid;
  ssize_t numRead;
  long pid, ppid;
  int statusFd;
  char buf[BUF_SIZ], statusFile[STATUS_FILE_MAX], command[BUF_SIZ];
  char *line;
  Bool parentFound, commandFound;

  proc = opendir(PROC_FS);
  if (proc == NULL) {
    pexit("opendir");
  }

  for (errno = 0; (procpid = readdir(proc)) != NULL; errno = 0) {
    /* skip `self` link */
    if (strncmp(procpid->d_name, "self", 4) == 0) {
      continue;
    }

    snprintf(buf, BUF_SIZ, "%s/%s/status", PROC_FS, procpid->d_name);
    statusFd = open(buf, O_RDONLY);

    if (statusFd == -1) {
      if (errno != ENOENT && errno != ENOTDIR) {
        pexit("open");
      }

      /* if the directory does not contain a status file or is not even a directory, move on */
      continue;
    }

    /* load the whole status file in memory to make sure field comparisons will work */
    numRead = read(statusFd, statusFile, STATUS_FILE_MAX);
    if (numRead == -1) {
      pexit("read");
    } else if (numRead == STATUS_FILE_MAX) {
      fprintf(stderr, "Your status file seems to be too big (> 10kB). Nothing we can do.\n");
      exit(EXIT_FAILURE);
    }

    if (close(statusFd) == -1) {
      pexit("close");
    }

    pid = atol(procpid->d_name);
    if (pid == 0) {
      fprintf(stderr, "FATAL: Invalid PID directory %s/%s/status\n", PROC_FS, procpid->d_name);
      exit(EXIT_FAILURE);
    }

    parentFound = commandFound = FALSE;
    line = strtok(statusFile, "\n");
    ppid = -1;
    while (line != NULL) {
      /* check for parent PID */
      if (strncmp(line, "PPid:", 5) == 0) {
        strncpy(buf, line + 6, BUF_SIZ);
        ppid = atol(buf);
        parentFound = TRUE;
      }

      /* check for command */
      if (strncmp(line, "Name:", 5) == 0) {
        strncpy(command, line + 6, BUF_SIZ);
        commandFound = TRUE;
      }

      line = strtok(NULL, "\n");
    }

    if (!parentFound || !commandFound) {
      fprintf(stderr, "Something seems wrong with the status file of process %ld. It is missing fields.\n", pid);
      exit(EXIT_FAILURE);
    }

    /* save process data in hierarchy data structure */
    strncpy(processes[pid].command, command, BUF_SIZ);

    if (ppid > 0) {
      processes[ppid].children[processes[ppid].children_count++] = pid;
    }
  }

  if (errno != 0) {
    pexit("readdir");
  }

  if (closedir(proc) == -1) {
    pexit("closedir");
  }
}

void
printTree(struct process *processes, int root, int level) {
  struct process rootp = processes[root];
  int i;

  /* properly ident according to the level in the hierarchy */
  for (i = 0; i < level; ++i) {
    printf("  ");
  }

  printf("- (%ld) %s\n", (long) root, processes[root].command);

  if (processes[root].children_count > 0) {
    ++level;

    /* print children */
    for (i = 0; i < rootp.children_count; ++i) {
      printTree(processes, rootp.children[i], level);
    }
  }
}
