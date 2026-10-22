# CodeCrafters Mini-Shell

A small POSIX-ish shell written in C++, built while working through the
[CodeCrafters "Build Your Own Shell"](https://codecrafters.io/challenges/shell) challenge.


## What it can do so far

- **Builtins:** `echo`, `exit`, `type`, `pwd`, `cd` (including `cd ~`)
- **External programs:** looks them up on your `PATH` and runs them with `fork` + `execvp`
- **Quoting:** single quotes, double quotes, and backslash escapes

```sh
$ echo 'hello    world'
hello    world
$ type echo
echo is a shell builtin
$ type ls
ls is /bin/ls
$ cd ~
$ pwd
/Users/you
$ exit
```

## Running it

You'll need CMake, a C++ compiler, and [vcpkg](https://vcpkg.io) (with `VCPKG_ROOT` set). Then:

```sh
./your_program.sh
```

That builds into `build/` and drops you at the prompt.

## Where things live

Everything is in [src/main.cpp](src/main.cpp): read a line, tokenize it, then either
run a builtin or go find the program on `PATH`.
