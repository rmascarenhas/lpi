~~~sh
#!/bin/cat -n
Hello world
~~~

  If the above file is `exec`ed, using any function of the family, then the command
built would be:

~~~console
/bin/cat -n <file> <args passed to exec>
~~~

  As a consequence, the output would be the file itself being print with line numbers
(the `-n` switch) along with any other file, if present, according to the arguments
passed to `exec`.
