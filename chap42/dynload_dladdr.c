/* dynload_dladdr.c - Prints function addresses from a dynamically loaded shared library.
 *
 * The `dladdr(3)` function is a GNU extension that, given a function address,
 * is capable of returning a `Dl_info` struct containing information about
 * the file where the function is defined.
 *
 * This program extends the `shlibs/dynload.c` listing from the chapter 43
 * of the Linux Programming Interface book, adding `dladdr` calls to inspect
 * its result
 *
 * Usage
 *
 *    $ LD_LIBRARY_PATH=. ./dynload_dladdr libx1.so libx1_f1
 *
 * Changes on the original `dynload.c` by: Renato Mascarenhas Costa
 */

#define _GNU_SOURCE /* dladdr(3) definition */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

static void helpAndExit(int status, const char *progname);

static void fatal(const char *message);
static void pexit(const char *fname);

int
main(int argc, char *argv[]) {
	void *libHandle;
	void (*funcp)(void);
	Dl_info funcInfo;
	const char *err;

	if (argc != 3)
		helpAndExit(EXIT_FAILURE, argv[0]);

	/* load the shared library and get a handle for later use */
	libHandle = dlopen(argv[1], RTLD_LAZY);
	if (libHandle == NULL)
		fatal(dlerror());

	/* search library for symbol named in argv[2] */
	(void) dlerror(); /* clear dlerror */
	*(void **) (&funcp) = dlsym(libHandle, argv[2]);
	err = dlerror();
	if (err != NULL)
		fatal(err);

	printf("Calling function %s\n", argv[2]);
	if (funcp == NULL)
		printf("%s is NULL\n", argv[2]);
	else
		(*funcp)();

	/* extract information about retrieved symbol */
	printf("\nGetting address information for %s:%s\n", argv[1], argv[2]);
	if (dladdr(funcp, &funcInfo) == 0)
		pexit("dladdr");

	printf("%20s %10s\n", "Pathname of shared object:", funcInfo.dli_fname);
	printf("%20s %10p\n", "Address where shared object is loaded:", funcInfo.dli_fbase);
	printf("%20s %10s\n", "Name of symbol overlapping address:", funcInfo.dli_sname == NULL ? "NULL" : funcInfo.dli_sname);
	printf("%20s %10p\n", "Exact address of symbol above:", funcInfo.dli_saddr);

	exit(EXIT_SUCCESS);
}

void
fatal(const char *message) {
	fprintf(stderr, message);
	exit(EXIT_FAILURE);
}

void
pexit(const char *fname) {
	perror(fname);
	exit(EXIT_FAILURE);
}

void
helpAndExit(int status, const char *progname) {
	FILE *stream = stderr;

	if (status == EXIT_SUCCESS)
		stream = stdout;

	fprintf(stream, "Usage: %s [lib-path] [func-name]\n", progname);
	exit(status);
}
