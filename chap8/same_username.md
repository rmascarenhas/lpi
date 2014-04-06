 ~~~c
printf("%s %s\n", getpwuid(uid1)->pwname,
                  getpwuid(uid2)->pwname);
~~~

The `printf(3)` function above will print the same user twice because the buffer returned
by the `getpwuid(3)` function (and related) is statically allocated, which means that
subsequent calls to the same function will overwrite the result of previous calls.
Hence, the username string that `printf(3)` will receive is the one for the last
evaluated call.

As an aside, the preceding code produces undefined behavior according to ISO/IEC 9899:1999
as per the following:

> The order of evaluation of the function designator, the actual arguments, and
> subexpressions within the actual arguments is unspecified, but there is a sequence point
> before the actual call.

Thus, the `printf` call above can either print the username for user `uid1` or `uid2` twice.
