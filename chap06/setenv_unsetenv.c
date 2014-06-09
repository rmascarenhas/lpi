/* setenv_unsetenv.c - Implement setenv(3) and unsetenv(3) in terms of getenv(3) and putenv(3).
 *
 * The setenv(3) and unsetenv(3) library functions present a slightly higher level
 * API for managing the process environment. They control the format of the name-value
 * pairs and also remove multiple definitions of an entry if present.
 *
 * This program implements both library functions in terms of the more raw getenv(3) and
 * putenv(3).
 *
 * Usage
 *
 *    $ ./setenv_unsetenv [-s NAME=VALUE] [-u NAME] [-g NAME]
 *
 *    Options:
 *      s - sets an environment variable. Takes an argument of the form NAME=VALUE. Note
 *          that consecutive set arguments will overwrite the previous values.
 *      u - unsets the environment variable with the passed name.
 *      g - gets the value of an environment variable.
 *
 * Example
 *
 *    $ ./setenv_unsetenv -s NAME=Renato -s LANG=C -u LANG -g NAME -g CODE
 *    Env var NAME set to Renato
 *    Env var LANG set to C
 *    Env var LANG unset
 *    Env var NAME value: Renato
 *    Env var CODE is not set.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE /* getopt function */

#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SU_BUF_SIZ
#define SU_BUF_SIZ 1024
#endif

typedef enum { FALSE, TRUE } Bool;

void helpAndLeave(const char *progname, int status);
void pexit(const char *fCall);

/* Gets the name of an environment variable given in the form NAME=VALUE and
 * stores it in the `buf` parameter passed.
 *
 * Returns 0 on success or -1 on error. */
int getEnvName(char *env, char *buf);

/* Gets the value of an environment variable given in the form NAME=VALUE and
 * stores it in the `buf` parameter passed.
 *
 * Returns 0 on success or -1 on error. */
int getEnvValue(char *env, char *buf);

int _setenv(const char *envname, const char *envval, int overwrite);
int _unsetenv(const char *envname);

extern char **environ;

int
main(int argc, char *argv[]) {
  int opt;
  char name[SU_BUF_SIZ], value[SU_BUF_SIZ];
  char *readValue;

  if (argc < 2) {
    helpAndLeave(argv[0], EXIT_FAILURE);
  }

  opterr = 0;
  while ((opt = getopt(argc, argv, "+s:u:g:")) != -1) {
    switch(opt) {
    case 's':
      if (getEnvName(optarg, name) == -1 || getEnvValue(optarg, value) == -1) {
        printf("Invalid format: %s\n", optarg);
        exit(EXIT_FAILURE);
      }

      if (_setenv(name, value, TRUE) == -1) {
        printf("Failed to set env var %s\n", name);
        pexit("_setenv");
      } else {
        printf("Env var %s set to %s\n", name, value);
      }

      break;

    case 'u':
      if (_unsetenv(optarg) == -1) {
        printf("Failed to unset env var %s\n", optarg);
        pexit("_unsetenv");
      } else {
        printf("Env var %s unset\n", optarg);
      }

      break;

    case 'g':
      readValue = getenv(optarg);
      if (readValue == NULL) {
        printf("Env var %s is not set\n", optarg);
      } else {
        printf("Env var %s value: %s\n", optarg, readValue);
      }

      break;

    case '?':
      helpAndLeave(argv[0], EXIT_FAILURE);
    }
  }

  return EXIT_SUCCESS;
}

void
helpAndLeave(const char *progname, int status) {
  FILE *stream;

  if (status == EXIT_SUCCESS) {
    stream = stdout;
  } else {
    stream = stderr;
  }

  fprintf(stream, "Usage: %s [-s NAME=VALUE] [-g NAME]\n", progname);
  exit(status);
}

void
pexit(const char *fCall) {
  perror(fCall);
  exit(EXIT_FAILURE);
}

int
getEnvName(char *env, char *buf) {
  char *p;
  int i;

  for (i = 0, p = env; *p != '=' && *p != '\0'; ++i, ++p) {
    buf[i] = *p;
  }

  buf[i] = '\0';

  return *p == '=' ? 0 : -1;
}

int
getEnvValue(char *env, char *buf) {
  char *p;
  int i;

  p = env;
  while (*p != '=' && *p != '\0') ++p;
  if (*p == '\0') return -1;

  for (i = 0, ++p; *p != '\0'; ++i, ++p) {
    buf[i] = *p;
  }

  buf[i] = '\0';

  return 0;
}

int
_setenv(const char *envname, const char *envvalue, int overwrite) {
  int envLength;
  char *envStr;
  long argMax;

  if (envname == NULL || *envname == '\0' || *envname == '=') {
    errno = EINVAL;
    return -1;
  }

  argMax = sysconf(_SC_ARG_MAX);
  envLength = strlen(envname) + strlen(envvalue) + 2; /* account for equal sign and terminating null byte */

  if ((long) envLength >= argMax) {
    errno = ENOMEM;
    return -1;
  }

  /* if there is an already set value for the variable and we are not
   * overwriting, ignore the request */
  if (getenv(envname) != NULL && !overwrite) {
    return 0;
  }

  envStr = malloc(envLength);
  if (envStr == NULL) {
    pexit("malloc");
  }

  if (sprintf(envStr, "%s=%s", envname, envvalue) == -1) {
    pexit("sprintf");
  }

  return putenv(envStr);
}

int
_unsetenv(const char *envname) {
  char **ep,    /* the environment pointer                */
       **epIdx; /* iterator for shifting environment data */
  char name[SU_BUF_SIZ];

  if (envname == NULL || *envname == '\0' || *envname == '=') {
    errno = EINVAL;
    return -1;
  }

  for (ep = environ; *ep != NULL; ++ep) {
    if (getEnvName(*ep, name) == -1) {
      return -1;
    }

    if (strncmp(envname, name, SU_BUF_SIZ) == 0) {
      /* shift all subsequent env vars in order to remove the current one */
      for (epIdx = ep; *epIdx != NULL; ++epIdx) {
        *epIdx = *(epIdx + 1);
      }
    }
  }

  return 0;
}
