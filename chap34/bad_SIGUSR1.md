~~~c
/* Call fork() to create a number of child processes, each of which
  remains in same process group as the parent */

/* Sometime later... */
signal(SIGUSR1, SIG_IGN); 	/* Parent makes itself immune to SIGUSR1 */

killpg(getpgrp(), SIGUSR1); 	/* Send signal to children created earlier */
~~~

  This code will work fine on most situations. However, there is a problem
when it is used within shell pipelines, as kindly hinted by the author. An unhandled
`SIGUSR1` causes the receiving process to terminate. Therefore, if this program
is used in a shell pipeline, all other programs in that command line will also
receive the signal (since they belong to the same process group as well) and can
be killed inadvertently.

  To cope with this undesired behavior, the application developer could, instead:

* Kept track of the process IDs of the children of the parent process and manually
sent the `SIGUSR1` signal to them at convenient time, or;

* Created a new process group for the children of the parent process. That way,
the parent process would be able to send the `SIGUSR1` signal to that process
group without the risk of interfering with neighbour processes within a shell pipeline.
