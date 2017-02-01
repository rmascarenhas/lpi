/* main.c - Demonstrates that `dlclose(3)` does not unloads a library if still in use.
 *
 * The `dlclose(3)` library function allows the caller to close a previously dynamically
 * opened shared library. However, if more than one library/program opens the same
 * library, the C library will keep the library loaded until the number of
 * references to a given library reaches zero.
 *
 * This program demonstrates such behaviour by:
 *
 * * loading `libx2` in this program
 * * loading `libx1` in this program
 * * as `libx1` uses `libx2`, if `main` uses `dlclose(3)` on `libx2`, then it will
 *   not be unloaded, since it is still required by `libx1`, which is in use
 *   by `main`.
 *
 * Compilation:
 *
 * 	  $ gcc -Wall -Wextra -o main main.c -ldl
 *
 * Usage
 *
 *    $ ./main
 *
 * Author: Renato Mascarenhas Costa
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

static void fatal(const char *message);
static void invoke(void *handle, const char *fname);

int
main(__attribute__ ((unused)) int argc, __attribute__ ((unused)) char *argv[]) {
	/* open both libraries, resolving all symbols immediately */
	void *libx1_handle,
		 *libx2_handle;

	libx1_handle = dlopen("libx1.so", RTLD_NOW);
	if (libx1_handle == NULL)
		fatal(dlerror());

	libx2_handle = dlopen("libx2.so", RTLD_NOW);
	if (libx2_handle == NULL)
		fatal(dlerror());

	/* invoke libx2_f2() */
	invoke(libx2_handle, "libx2_f2");

	/* invoke libx1_f1() */
	invoke(libx1_handle, "libx1_f1");

	printf("Going to `dlclose(3)' libx2, it should not be unloaded since libx1 depends on it.\n");
	dlclose(libx2_handle);

	printf("Main is finished.\n");
	exit(EXIT_SUCCESS);
}

/* invoker of a function of void (*fn)(void) signature */
void
invoke(void *handle, const char *fname) {
	const char *err;
	void (*funcp)(void);

	(void) dlerror(); /* clear dlerror */
	*(void **) (&funcp) = dlsym(handle, fname);
	err = dlerror();

	if (err != NULL)
		fatal(err);

	(*funcp)();
}

void
fatal(const char *message) {
	fprintf(stderr, message);
	exit(EXIT_FAILURE);
}
