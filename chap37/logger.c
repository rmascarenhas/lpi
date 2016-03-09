/* logger.c - a simplified implementation of the logger(1) utility.
 *
 * The `logger(1)` utility is a command-line interface for the syslog
 * facility, which is a centralised service receiving logs from multiple
 * sources and being able to distribute logged messages to different
 * locations (including communicating to different machines over the
 * network.)
 *
 * This program will log the given message to syslog, allowing the user
 * to specify the ident(ifier) and the log level. For convenience, the
 * `LOG_PERROR` is used when logging so that the logged message can
 * be checked on standard error.
 *
 * Usage:
 *
 *   $ ./logger -i logger_test -l info "my log message"
 *   Options:
 *
 *   	-i: sets the program ident. Defaults to `_LOGGER`.
 *   	-l: sets the log level. Defaults to `info`. Accepted
 *   	values are: `emerg`, `alert`, `crit`, `err`, `warning`,
 *   	`notice`, `info` and `debug`.
 *
 * Author: Renato Mascarenhas Costa
 */

#define _XOPEN_SOURCE /* getopt function */

#include <syslog.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_IDENT     ("_LOGGER")
#define DEFAULT_LOG_LEVEL (LOG_INFO)

#ifndef BUF_SIZ
#  define BUF_SIZ (1024)
#endif

static void helpAndLeave(const char *progname, int status);
static int parse_level(const char *level, int *out);

int
main(int argc, char *argv[]) {
	if (argc < 2)
		helpAndLeave(argv[0], EXIT_FAILURE);

	int opt;
	int level   = DEFAULT_LOG_LEVEL;
	char *ident = DEFAULT_IDENT;
	char *message;

	/* command-line parsing */
	opterr = 0;
	while ((opt = getopt(argc, argv, "+i:l:")) != -1) {
		switch(opt) {
			case '?': helpAndLeave(argv[0], EXIT_FAILURE); break;
			case 'i': ident = optarg; break;
			case 'l':
					  if ((parse_level(optarg, &level)) == -1)
						  helpAndLeave(argv[0], EXIT_FAILURE);
					  break;
		}
	}

	message = argv[optind];
	if (message == NULL)
		helpAndLeave(argv[0], EXIT_FAILURE);

	/* sets default syslog options */
	openlog(ident, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

	/* write message to the log */
	syslog(level, "%s", message);

	return EXIT_SUCCESS;
}

/* Parses a string indicating a log level and assigns the corresponding log level
 * constant to the `out` parameter.
 *
 * emerg   => LOG_EMERG
 * alert   => LOG_ALERT
 * crit    => LOG_CRIT
 * err     => LOG_ERR
 * warning => LOG_WARNING
 * notice  => LOG_NOTICE
 * info    => LOG_INFO
 * debug   => LOG_DEBUG
 *
 * Returns 0 if the level was recognised or -1 otherwise.
 */
static int
parse_level(const char *level, int *out) {
	if (!strncmp(level, "emerg", BUF_SIZ)) {
		*out = LOG_EMERG;
	} else if (!strncmp(level, "alert", BUF_SIZ)) {
		*out = LOG_ALERT;
	} else if (!strncmp(level, "crit", BUF_SIZ)) {
		*out = LOG_CRIT;
	} else if (!strncmp(level, "err", BUF_SIZ)) {
		*out = LOG_ERR;
	} else if (!strncmp(level, "warning", BUF_SIZ)) {
		*out = LOG_WARNING;
	} else if (!strncmp(level, "notice", BUF_SIZ)) {
		*out = LOG_NOTICE;
	} else if (!strncmp(level, "info", BUF_SIZ)) {
		*out = LOG_INFO;
	} else if (!strncmp(level, "debug", BUF_SIZ)) {
		*out = LOG_DEBUG;
	} else {
		return -1;
	}

	return 0;
}

static void
helpAndLeave(const char *progname, int status) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [-i ident] [-l level] message\n", progname);
	exit(status);
}
