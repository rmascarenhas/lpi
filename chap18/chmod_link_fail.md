~~~c
mkdir("test", S_IRUSR | S_IWUSR | S_IXUSR);
chdir("test");
fd = open("myfile", O_RDWR | O_CREAT, S_IRUSR | IW_USR);
symlink("myfile", "../mylink");
chmod("../mylink", S_IRUSR);
~~~

Symbolic (soft) links hold a name to the file they point to. When symlinks are
resolved, this name is considered to be _in the same directory as the link itself_.
Thus, the above `chmod(2)` call will try to dereference the `../mylink` to
`../myfile`, but it does not exist (`myfile` was created inside `test`, and that is
the working directory).

This is why the above `chmod(2)` call would fail with error set to `ENOENT`.
