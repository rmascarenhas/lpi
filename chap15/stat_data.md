The timestamps stored for each file are:

* Access time: the last time the file was read;
* Modification time: the last time the file contents were modified;
* Status change time: the last time the file metadata was modified.

The `stat(2)` system call reads a given file metadata information, which is
localed in its corresponding `i-node`. As such, it does _not_ change the file
content or metadata and that explains why the timestamps remain the same.

This can be verified by running consecutive calls to `stat(1)` and checking
the timestamps.
