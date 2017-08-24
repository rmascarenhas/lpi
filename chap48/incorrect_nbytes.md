> Explain why the program in Listing 48-3 incorrectly reports the number
> of bytes transferred if the `for` loop is modified as follows:

~~~c
for (xfrs = 0, bytes = 0; shmp->cnt != 0; xfrs++, bytes += shmp->cnt) {
	reserveSem(semid, READ_SEM); /* Wait for our turn */

	if (write(STDOUT_FILENO, shmp->buf, shmp->cnt) != shmp->cnt)
		fatal("write");

	releaseSem(semid, WRITE_SEM); /* Give the writer a turn */
}
~~~

According to the manual pages of the `shmget(2)` system call on Linux:

> When a new shared memory segment is created, its contents are initialized
> to zero values, [...]

Therefore, the struct pointed to by `shmp` has a `0` `cnt` field, and never
enters the loop.
