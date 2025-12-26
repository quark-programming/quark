<div align="center">
    <img alt="Quark Programming Language" src="assets/title.png" width="600px" />

[Homepage](https://quar.k.vu) | [Docs](https://quar.k.vu/docs.html)
</div>

> This langauge is in early stages of development, everything is subject to change

## Building the Compiler

To build the Compiler (default: `qc`):

```sh
$ make build
or 
$ make build out=PATH
```

# Compiling a Quark File (`.qk`) to C:

```sh
$ ./qc -h
$ ./qc <path> -o <out-path>
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

See the [standard library](lib/) for examples of the language in use or visit the [docs](https://quar.k.vu/docs.html).


