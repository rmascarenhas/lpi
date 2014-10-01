~~~sh
PATH=/usr/local/bin:/usr/bin:/bin:./dir1:./dir2
~~~

  Both `./dir1` and `./dir2` have a `xyz` file, and the `execlp(3)` function will
look for a program by sequentially searching each directory present in the `PATH`
environment variable.

  For this reason, the `xyz` file is first found on `./dir1`. However, the file present there
does not have execute permission for any user, and hence it is skipped. The function
later finds the `./dir2/xyz` file which has execute permissions, causing this file to
be executed.
