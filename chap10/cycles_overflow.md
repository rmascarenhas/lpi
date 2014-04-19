~~~c
sysconf(_SC_CLK_TICK); /* => 100 */
~~~

The limit for an unsigned int is `4294967295`, that is, we can reliably calculate
times in the range up to that amount of clock ticks. As we have 100 clock ticks
per second, the interval is equivalent to 42949672 seconds, i.e., approximately
497 days. However, as the `times` system call returns the distance considering an
arbitrary point in the past, this amount is usually much smaller than that.

For the `clock` system call, the return value is in units of `CLOCKS_PER_SEC`,
which is fixed at 1 million by POSIX.1. Thus, we can reliably measure 4294967295
divided by 1 million seconds, and that is approximately 70 minutes.
