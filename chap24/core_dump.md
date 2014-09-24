> Assuming that the we can modify the program source code, how could we get
> a core dump of a process at a given point in the program, while letting the
> process continue execution?

One way to achieve that would be using the `fork(2)` system call. As the child's memory
space is an exact copy of that of the parent, we can save the core dump in the child
while continuing normal execution in the parent:

~~~c
if (fork() == 0) {
  /* get the core dump (via SIGQUIT, using a debugger, or by any other means) */
  _exit(0); /* once that is done, the child can terminate */
}

/* parent process code can continue execution normally */
~~~
