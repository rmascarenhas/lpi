~~~c
static void
tstpHandler(int sig)
{
	/* ... */

	if (sigprogmask(SIG_UNBLOCK, &tstpMask, &prevMask) == -1)
		errExit("sigprocmask");

	printf("Caught SIGTSTP\n");

	if (signal(SIGTSTP, SIG_DFL) == SIG_ERR)
		errExit("signal")

	raise(SIGTSTP);

	/* ... */
}
~~~

  In the preceding piece of code, the `sigprocmask(3)` call was moved to the top
of the function. That simple change makes a subtle race condition possible: if
the process receives a new `SIGTSTP` signal after it is unblocked but before
setting the disposition to its default, the signal handler would be invoked again.

  That could be quite troublesome because the `printf(3)` call is not async-
safe. In a more realistic scenario, as the one discussed in the chapter, of a
program that changes the terminal settings, this could cause the program to change
terminal settings more than once, which would lead to bugs that are hard to identify.
