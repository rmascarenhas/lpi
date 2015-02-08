~~~c
childPid = fork();
if (childPid == -1)
  errExit("fork1");
if (childPid == 0) { /* Child */
  switch (fork()) {
    case -1: errExit("fork2");

    case 0: /* Grandchild */
      /* -------- Do real work here ---------- */
      exit(EXIT_SUCCESS);

    default: /* Make grandchild an orphan */
      exit(EXIT_SUCCESS);
  }
}

/* Parent falls through to here */
if (waitpid(childPid, &status, 0) == -1)
  errExit("waitpid");

/* Parent carries on to do other things */
~~~

  The above snippet sets up a grandchild to perform some task while the parent
moves on with execution as long as the forking has finished.

  This is useful if some not really critical but potentially expensive work has
to be performed without blocking execution. Using this technique, the parent can
continue its work while the grandchild carries out such work.

  The main advantage of the technique, however, is the fact that the grandchild
_is made an orphan_. As a consequence, its parent will then be the `init` process,
which will be responsible for clearing out the grandchild's resources when it is finished.
This brings the parent the advantage of not having to check on the status of the
grandchild in order to clean it from the process table - this will be taken care
of automatically.
