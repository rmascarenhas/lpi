~~~c
static void *
threadFunc(void *arg) {
  struct someStruct *pbuf = (struct someStruct *) arg;

  /* Do some work with structure pointed to by 'pbuf' */
}

int
main(int argc, char *argv[]) {
  struct someStruct buf;

  pthread_create(&thr, NULL, threadFunc, (void *) &buf);
  pthread_exit(NULL);
}
~~~

  The code above attempts to pass a buffer to the created thread so that
it could do some work with it. However, as it can be seen, the main thread
itself later calls `pthread_exit(3)`, which, unlike `exit(2)`, will not
terminate the remaining threads.

  As a consequence, the address space in which the main thread's stack was
located can be reused and any reference to data in that region becomes undefined.
And that is precisely the problem with the code above: if the main thread
performs `pthread_exit(3)` before the child is done (and chances are this will
happen at some point), then the program will depend on undefined behavior and
is likely to produce wrong results.

  A solution to solve the above situation would be placing the `someStruct`
data in the process' heap, since it is shared among threads (though this could
bring other problems if there are other threads using the same shared memory).
