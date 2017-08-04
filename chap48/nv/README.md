# nv(1)

`nv` is a tool that allows the concurrent execution of scripts that share a
name/value data structure.

It started out as an implementation of exercise `48-5` of **The Linux Programming
Interface** book - after a lot of yak shaving, it took its present form.

### What it is

`nv(1)` is a way to provide a name/value data structure that can be shared across
multiple processes. Manipulation of this data structure is done via _scripts_
(for no good reason.) These scripts are just text files (see the `example_script.txt`
file included) containing a collection of pre-defined commands.

`nv(1)` processes can execute scripts based on the same memory segment via the use
of the `-m` flag (see Usage section below.)

### Scripts

The script language executed by `nv(1)` is simple (at least implementation wise):
just a series of commands. No flow control is built into it. Aditionally, there are
no data types: everything (names, values, variables) are strings.

List of supported commands:

##### `set [name] [value]`

As said previously, `nv` is ultimately a way to share a name/value data structure
among processes. The `set` command associates a given name to a given value. If the
name already exists, it is updated.

##### `setifnone [name] [value]`

Very similar to `set`, with the difference that the name is only associated with
the value if it did not exist before. This operation is atomic.

##### `get [name]`

Looks up the for the given name. Every `get` command updates a special variable,
`$_` (see `assign` for more on variables.) Future commands can make use of this
variable to retrieve the value associated with a given name. If the name requested
does not exist and the `$_` variable had a previous value, that value is lost.

##### `assign [var] [value]`

Scripts can contain _variables_. Variables are per-process concepts: a variable set
within one `nv` process exists only within that process. Variables can be accessed
by using the `$var` syntax: a dollar sign and the name of the variable. Therefore, the
following construct is possible:

```
assign name Renato
get $name
```

In the lines above, we would look up the `Renato` name in the shared name/value
data structure.

Note that the `value` of a variable can be a series of strings. These are concatenated
to become a single string separated by a single blank space:

```
// valid
assign info The name of this file is README.md
```

##### `delete [name]`

Removes any association with the name given from the shared data structure. If
there was no value associated with requested name, this command has no effects.

##### `print [message]`

This command prints the given message to the standard output. As with the `assign`
command, multiple arguments can be passed to the `print` command, including variables:

```
assign file README.md
print The name of this file is $file
```

##### Comments

C-style double-slash comments (`//`) are allowed, both taking an entire line,
or inline with a command:

```
// script.txt
//
// This is an example of a comment.
// It can span multiple lines.

assign name nv // comments may also be on the same line
```

##### Important rules

* Everything is a string
* Trying to use a variable which is not defined results in a runtime error: script
execution halts and the process exists with an unsuccessful exit status.

### Usage

`nv` uses System V shared memory segments in order to allow different processes
to communicate and share data. If no arguments are passed, `nv` will create a
short-lived shared memory segment, used only the execution of the script. However,
there are a number of command-line arguments that can be passed which alter
the behaviour of this program:

`-p`
Creates a _persistent_ shared memory segment - that is, one that is not deleted
and can be used as the memory block for future `nv` processes. When this flag is
passed, the output of `nv` is the identifier of the memory segment, which can be
reused by passing this identifier to the `-m` flag.

`-m [id]`
Receives the identifier of a previously created shared memory segment (using the
`-c` flag) and uses this block of memory to perform writes/updates and name lookups.
Notice that the identifier given here **must** be created with the `-p` option, since
`nv` will insert important metadata information on the memory block to prepare for
future usage.

`-c [max-pairs]`
Indicates the maximum number of name/value pairs to be allowed in the block of memory.
Can be used in conjunction with the `-p` option. If not passed, it defaults to 1,000.

`-d [id1] [id2] ... [idN]`
Deletes the shared memory segment (and associated resources) with the identifier(s)
given. Used when a block of shared memory created with `-p` is no longer needed.

`-h`
Prints a help message, with the list of accepted command line arguments and their
usage.

### Building and running

This distribution contains a `Makefile`. To compile all sources, run:

```console
$ make
```

If all goes well, an `nv` executable should be created as a result. It can then
be used to process script files.

**Running the example script included:**

```console
$ ./nv example_script.txt
Shared memory segment: 0x7f7777b8a000

The value of os is linux
The value of name is nv
os is now (null)
Goodbye.
```

As it can be seen, the first line of the execution is:

```
Shared memory segment: 0x7f7777b8a000
```

This indicates the address, within the process virtual memory space,
of the block of memory where the name/value data structure is maintained.

If no file is passed to `nv`, the script is read from standard input:

```console
$ ./nv
print Hello, world!
^D
Shared memory segment: 0x7f4580f23000

Hello, world!
