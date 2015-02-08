/* execlp.c - an implementation of the execlp(3) function in terms of execve(2).
 *
 * The execlp(3) function provides a different API for exec'ing a program:
 *
 *    * the program name is searched in every directory of the PATH environment
 *      variable, as happen in a shell environment;
 *
 *    * the list of arguments passed to the command (argv) is given as a series of
 *      strings to the function, without the need to manually build the NULL terminated
 *      array of strings;
 *
 *    * the environment passed is the same of the calling process.
 *
 * Usage
 *
 *    $ ./execlp <command>
 *
 *    comamnd - the command to be executed
 *
 * This program will pass "Linux", "Programming", "Interface" as arguments to
 * the given command.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE

#include <limits.h>
#ifndef PATH_MAX
#  include <linux/limits.h>
#endif

#include <unistd.h>
#include <errno.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHELL ("/bin/sh")
#define PATH_SEP (":")
#define MAX_ARGS (1024)
#define MAX_ENVS (MAX_ARGS)

enum { FALSE, TRUE } Bool;

int _execlp(const char *file, const char *arg, ...);

static void helpAndLeave(const char *progname, int status);
static void pexit(const char *fCall);

int
main(int argc, char *argv[]) {
  if (argc != 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  _execlp(argv[1], "Linux", "Programming", "Interface", (char *) NULL);

  /* if we get here, something went wrong with _execlp */
  pexit("_execlp");

  exit(EXIT_FAILURE);
}

/* prepends a new string given as first argument to this function to the NULL-
 * terminated list of strings given in the second argument. */
static void
prependArg(const char *new, const char *args[]) {
  const char **p;
  int nargs, i;

  nargs = 0;
  for (p = args; *p != NULL; ++p, ++nargs);

  for (i = nargs; i >= 0; --i) {
    args[i + 1] = args[i];
  }

  args[0] = new;
}

/* when the PATH environment variable is not defined, it is assumed to be
 * the current directory plus the content of _CS_PATH */
static void
getPath(char *buffer, int size) {
  char *envPath, *defaultPath;
  int defaultPathLen;

  envPath = getenv("PATH");
  if (envPath == NULL) {
    defaultPathLen = confstr(_CS_PATH, NULL, 0);
    defaultPath = malloc(defaultPathLen);
    confstr(_CS_PATH, defaultPath, defaultPathLen);

    snprintf(buffer, size, ".:%s", defaultPath);
    free(defaultPath);
  } else {
    strncpy(buffer, envPath, size);
  }
}

/* tries to pass the file given as first argument to the shell. Useful with the
 * execlp(3) semantics of trying to give a file that failed to execute to the shell
 * before giving up */
static int
tryShell(const char *argv[], char * const *envp) {
  prependArg(SHELL, argv);
  return execve(SHELL, (char * const *) argv, envp);
}

int
_execlp(const char *file, const char *arg, ...) {
  va_list arguments;
  const char *a;
  const char *argv[MAX_ARGS];
  int argc;
  long argmax, byteCount;

  extern char **environ;

  /* stage 1: parse arguments given as a list in the function call to build
   * the argv array */
  va_start(arguments, arg);

  a = arg;
  argv[0] = file;
  argc = 1; /* account for program name */
  byteCount = 0;
  argmax = sysconf(_SC_ARG_MAX);

  while (byteCount <= argmax && a != NULL) {
    argv[argc] = a;
    ++argc;

    byteCount += strlen(a) + 1; /* account for the NUL byte */

    a = va_arg(arguments, const char *);
  }
  va_end(arguments);

  if (byteCount > argmax) {
    errno = E2BIG;
    return -1;
  }

  argv[argc] = NULL;

  /* stage 2: execution */

  /* if the file name contains a slash, it is either a relative or absolute
   * path. In either case, we should try to execute the it immediately
   * with no further processing */
  if (strchr(file, '/')) {
    execve(file, (char * const *) argv, environ);

    /* if we get to this point, something went wrong - and if the error happens
     * to be ENOEXEC, we try to execute the given command with the shell */
    if (errno == ENOEXEC) {
      return tryShell(argv, environ);
    }

    return -1;
  }

  /* PATH lookup */
  char path[PATH_MAX], progname[PATH_MAX], *prefix;
  int eacces = FALSE;
  getPath(path, PATH_MAX);

  prefix = strtok(path, PATH_SEP);
  while (prefix != NULL) {
    snprintf(progname, PATH_MAX, "%s/%s", prefix, file);
    execve(progname, (char * const *) argv, environ);

    /* if we get to this point, something went wrong when execing the progname.
     * Special semantics:
     *
     *    * EACCES  - the file was not found. We keep searching for the file in
     *                the other path prefixes, but if we did not find one at the
     *                end, this function errors out with EACCES.
     *
     *    * ENOEXEC - something went wrong when trying to execute the file. Maybe
     *                a shared library could not be found, or the file was a script
     *                with an invalid interpreter. As a last resort, we try to
     *                execute the file with the shell and if that fails, no more
     *                searching is attempted and the function exits with an error.
     */
    if (errno == EACCES) {
      eacces = TRUE;
    } else if (errno == ENOEXEC) {
      return tryShell(argv, environ);
    }

    prefix = strtok(NULL, ":");
  }

  /* if we got to this point, there was no file in any of the path prefixes, or
   * there was a broken binary file or script. In either case, we must finish
   * with an error */
  if (eacces) {
    errno = EACCES;
  }

  return -1;
}

static void
helpAndLeave(const char *progname, int status) {
  FILE *stream = stderr;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  }

  fprintf(stream, "Usage: %s <command>\n", progname);
  exit(status);
}

static void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}
