For this exercise, the `pstree.c` program, from chapter 12, is going to be used
to demonstrate the difference in final binaries size when programs are statically
compiled.

The standard, dynamic way of compiling it is to just invoke `gcc` (the default is
to produce a dynamic binary):

~~~console
% gcc -O2 -Wall -Wextra -g -o pstree pstree.c
~~~

Producing a static binary is as easy as adding the `-static` option to `gcc`:

~~~console
% gcc -O2 -Wall -Wextra -static -g -o pstree-bin pstree.c
~~~

A comparison of the sizes of the produced binaries show that the static file
is almost 30 times larger than the dynamic one, since it includes the C library
embedded on it:

~~~console
% du -sh *
24K  pstree
868K pstree-bin
8.0K pstree.c
~~~

`ldd` shows the dynamic libraries `pstree` depends on, whereas it reports that
`pstree-bin` is not a dynamically linked binary:

~~~console
% ldd pstree
  linux-vdso.so.1 =>  (0x00007fffe4dde000)
  libc.so.6 => /lib64/libc.so.6 (0x00007fe71a111000)
  /lib64/ld-linux-x86-64.so.2 (0x00007fe71a4eb000)
~~~

~~~console
% ldd pstree-bin
  not a dynamic executable
~~~
