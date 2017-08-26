/* rlimit_memlock_demo.c - Demonstrates RLIMIT_MEMLOCK enforcement.
 *
 * Memory locking allows a process to tell the kernel lock a certain part
 * of its virtual address space into physical memory (that is, not to swap it
 * to disk or wherever swapfs is mounted.)
 *
 * However, unprivileged processes can only lock up to RLIMIT_MEMLOCK bytes.
 * This program demonstrates that the limit is enforced by setting this limit
 * to a predefined number of pages (RLM_DEMO_PAGES), and then allocating and locking
 * one page at a time, until it fails.
 *
 * Usage
 *
 *    $ ./rlimit_memlock_demo
 *
 * Author: Renato Mascarenhas Costa
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RLM_DEMO_PAGES /* allow the compiler to override it */
#	define RLM_DEMO_PAGES (5)
#endif

static void pexit(const char *fCall);

int
main() {
	int i, j;
	long pagesize;
	void *mems[RLM_DEMO_PAGES];
	struct rlimit rl;

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1)
		pexit("sysconf");

	/* set soft and hard limits to the same number of bytes */
	rl.rlim_cur = RLM_DEMO_PAGES * pagesize;
	rl.rlim_max = RLM_DEMO_PAGES * pagesize;

	printf("Setting RLIMIT_MEMLOCK to %d pages\n", RLM_DEMO_PAGES);
	if (setrlimit(RLIMIT_MEMLOCK, &rl) == -1)
		pexit("setrlimit");

	for (i = 0; ; i++) {
		printf("-> Page %d: ", i+1);

		mems[i] = malloc(pagesize);
		if (mems[i] == NULL)
			pexit("malloc");

		if (mlock(mems[i], pagesize) == -1) {
			printf("%s\n", strerror(errno));
			break;
		} else {
			printf("OK\n");
		}
	}

	for (j = 0; j <= i; j++)
		free(mems[j]);

	exit(EXIT_SUCCESS);
}

static void
pexit(const char *fCall) {
	perror(fCall);
	exit(EXIT_FAILURE);
}
