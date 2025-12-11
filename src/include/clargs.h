#ifndef CLARGS_H
#define CLARGS_H

#include <stdio.h>
#include <stdlib.h>
#include "helpers.h"

#define panicf(fmt...) (fprintf(stderr, "\33[31;1merror:\33[0m " fmt), exit(EXIT_FAILURE))

extern int argc;
extern const char** argv;

const char* clname(int local_argc, nonnull const char** local_argv);

int clflag();

const char* clarg();

#endif
