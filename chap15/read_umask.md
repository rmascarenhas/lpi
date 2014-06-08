There is no way to retrieve the process umask without actually changing it once.
One approach to the problem is setting a value to the umask to get the current value
and then immediatelly set it back. That is actually the way of performing this
operation suggested by the GNU C Library:

~~~c
mode_t mask = umask(0);
umask(mask);
~~~

It is clear, though, that this solution is not ideal since it is not reentrant.

`glibc` provides an extension function called `getumask` which deals with umask
retrieving in a reentrant way. The signature for the function is:


~~~c
#define _GNU_SOURCE /* in order to get the function definition */
#include <sys/types.h>
#include <sys/stat.h>

mode_t getumask(void);
~~~
