~~~c
int
main(int argc, char *argv[]) {
  printf("Hello world");
  execlp("sleep", "sleep", "0", (char *) NULL);
}
~~~

  Running this program, it is noticed that it produces no output.

  The reason for that is that, by default, the standard library performs IO
buffering. Furthermore, when the standard output is a terminal, `stdio` performs
line-buffering and when it is a file, buffering happens in blocks.

  When the above code is run, the string passed to `printf` is actually kept on
userspace buffers, in the process memory region. When the call to `execlp` later
succeeds, that memory is overwritten, and the string is never sent to the kernel.

  To work around the issue, one could add a new line character at the end of the
`Hello world` string, or just disable buffering on standard output, as in:

~~~c
setbuf(stdout, NULL);
~~~
