<div align="center">
    <img alt="Quark Programming Language" src="assets/title.png" width="600px" />

[Homepage](https://quar.k.vu) | [Docs](https://quar.k.vu/docs.html)
</div>

> [!IMPORTANT]
> This langauge is in early stages of development, everything is subject to change, when building the compiler make
> sure to use the [latest release](https://github.com/quark-programming/quark/releases) You can see and submit issues
> on the repository's [issues page](https://github.com/quark-programming/quark/issues)

## Building the Compiler

To build the Compiler (default: `qc`):

```sh
make build # or
make build OUT=/path/to/executable
```

# Compiling a Quark File (`.qk`) to C:

```sh
./qc /path/to/source.qk -o /path/to/output.c
```

# Getting Started
#### Hello World
```quark
print("Hello World");
```
#### Generics 
```quark 
T add<T>(T a, T b) {
    return a + b;
}
```


## Learn More

See the [standard library](lib) for examples of the language in use or visit the [docs](https://quar.k.vu/docs.html).


