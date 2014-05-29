~~~c
printf("If I had more time, \n");
write(STDOUT_FILENO, "I would have written you a shorter letter.\n", 43);
~~~

I/O handling functions and system calls perform buffered operations
for increased performance. The `printf(3)` function is a library function that
has its own buffers that reside at user level. The kernel also buffers I/O so
that it does not have to write to the disk on every call.

By default, when the output file is a terminal, the writes using the `printf`
function are `line-buffered`, i.e, whenever an end-of-line character is found,
the buffers are flushed from the user memory and added to kernel space. However,
when it is not a terminal, then the contents are only flushed when there is no
more space at the buffer (or the file is closed).

That is: if the standard output of the program above is a terminal, then the first
call to `printf` will flush its buffer to the kernel since it finds the terminating
end-of-line character, and the output would be in the same order as in the
statements. However, if the output is a disk file, the `stdio` buffers would not
be flushed (since they would not be full with such a short string) and the
contents of the `write(2)` system call would hit the kernel buffers first,
causing it to be flushed to the disk before the contents of the `printf` call.


### When `stdout` is a terminal

~~~
If I had more time,
I would have written you a shorter letter.
~~~

### When `stdout` is a disk file

~~~
I would have written you a shorter letter.
If I had more time,
~~~
