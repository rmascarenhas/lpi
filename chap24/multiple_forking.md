~~~c
fork();
fork();
fork();
~~~

After each call to `fork()`, the number of processes doubles. This happens because
both the parent and the child share the same text data, so they will run the same
code, doubling the total number of process after each call.

The process is more easily seen by visualing the processes after each call to `fork()`.
Prior to any call, there is only one process, the parent process:

![the parent process](https://cloud.githubusercontent.com/assets/613784/4394350/c0ae0d84-4420-11e4-8f94-36bd6f746433.png)

First call to `fork()`:

![](https://cloud.githubusercontent.com/assets/613784/4394374/e8c55ebc-4420-11e4-88a3-15bce5e7ad96.png)

Second call to `fork()`:

![](https://cloud.githubusercontent.com/assets/613784/4394384/f8d57468-4420-11e4-9518-580927203e3d.png)

Third and last call to `fork()`:

![](https://cloud.githubusercontent.com/assets/613784/4394390/149f1bc2-4421-11e4-80bc-c7a06b586124.png)

Thus, after the three calls, a total of **8 processes** will have been created.
