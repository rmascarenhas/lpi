~~~console
$ touch file
$ chmod +x file
$ chmod u+s file
$ ls -l file
-rwsrwxr-x. 1 rmc 0 Mar 15 21:00 file*
$ echo change >>file
$ ls -l file
-rwxrwxr-x. 1 rmc 7 Mar 15 21:00 file*
~~~

As can be observed in the sequence of commands above, even if an executable file
has its set-user-ID bit set, a change on the file's contents cause it to be
reset.

The definition of `CAP_FSETID` on `capabilities(7)` provides a concrete
documentation of this fact.

The rationale behind it is that a program is trusted to be executed with
its set-user-ID the way it is configured. As soon as there is a (potentially
malicious) change to the file, there is no security guarantees that the
privileges will not be exploited. This is why set-user-ID (especially set-user-ID-root)
programs should never be publicly writable.
