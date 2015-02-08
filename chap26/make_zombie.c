/* make_zombie.c - demonstrates how to create a zombie process.
 *
 * This program creates a child process that immediately terminates, without the
 * parent successfully issuing a `wait` call. This constitutes what is called
 * a "zombie process", it is already finished but its information must be kept
 * in the kernel's process table in case the parent needs the information later
 * with a `wait` system call.
 *
 * This program also shows that zombie processes cannot be "killed" (since they are
 * already finished, and are not affected by signals, even SIGKILL.
 *
 * Usage
 *
 *    $ ./make_zombie
 *
 * Adapted from Listing 26-4 of the book The Linux Programming Interface.
 */

#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#define CMD_SIZE (200)

static volatile sig_atomic_t gotSigchld = 0;

static void chldHandler(int signal);

static void logInfo(const char *who, const char *message);
static void pexit(const char *fCall);

int
main(__attribute__((unused)) int argc, char *argv[]) {
  char cmd[CMD_SIZE];
  pid_t childPid;

  /* set up a handler for SIGCHLD */
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = chldHandler;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    pexit("sigaction");
  }

  setbuf(stdout, NULL); /* disable buffering of stdout */

  logInfo("Parent", "creating child");

  switch (childPid = fork()) {
    case -1:
      pexit("fork");
      break;

    case 0:
      /* child: immediately exits to become zombie */
      logInfo("Child", "terminating");
      _exit(EXIT_SUCCESS);

    default:
      /* parent: waits for children to be finished and then print information */

      /* ignore other signals if they arrive while we are waiting for the child */
      while (!gotSigchld) {
        pause();
      }

      /* child is now a zombie process */
      snprintf(cmd, CMD_SIZE, "ps | grep %s", basename(argv[0]));
      system(cmd);

      /* try to kill zombie */
      if (kill(childPid, SIGKILL) == -1) {
        pexit("kill");
      }

      logInfo("Parent", "SIGKILL sent, giving some time for it to take effect");

      sleep(3); /* give child a chance to react to the signal */
      system(cmd);
  }

  /* only parent process should reach this point */
  exit(EXIT_SUCCESS);
}

static void
chldHandler(__attribute__((unused)) int signal) {
  gotSigchld = 1;
}

static void
logInfo(const char *who, const char *message) {
  char buf[BUFSIZ];
  int size;

  size = snprintf(buf, BUFSIZ, "[%s %ld] %s\n", who, (long) getpid(), message);
  if (write(STDOUT_FILENO, buf, size) == -1) {
    pexit("write");
  }
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
