<div align="center">
    <img alt="Quark Programming Language" src="assets/title.png" width="50%" />
</div>

> This langauge is in early stages of development, everything is subject to change

## Quick Start

To build the compiler (default: `qc`):

```sh
$ make build
or 
$ make build out=PATH
```

Compile a `.qk` file to C:

```sh
$ ./qc main.qk -o main.c
```

## Examples

See source files in [lib/](lib/) for examples or the current [test file](test/main.qk).

## Hello World

```quark
import lib::io;

print(str::from("Hello World"));
```
