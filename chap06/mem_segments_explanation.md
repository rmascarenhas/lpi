Even though that program *declares* an array of 10MB, that storage unit is destined
to the **uninitialized data segment (bss)**, since no initialization is performed in the code.

Before starting the program, the system will initialize the data segment with zeroes.
This way, the executable file itself just needs to record the address and size of the
array, which is then actually allocated by the program loader at run time.
