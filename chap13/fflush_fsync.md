~~~c
fflush(fp);
fsync(fileno(fp));
~~~

The first line of the snippet above will flush all the buffers located at user
land stored by the `stdio` library. The flushed content will be written to the
kernel buffers via the `write(2)` system call.

The following statement however, causes the kernel buffers to also be flushed.
Particularly, the `fsync` call will flush not only the buffers for the contents
of the file, but also all related metadata will be flushed to disk, thus ensuring
file integrity.
