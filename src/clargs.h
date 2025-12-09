#ifndef CLARGS_H
#define CLARGS_H

#include <stdio.h>
#include <stdlib.h>

#define panicf(fmt...) (fprintf(stderr, "\33[31;1merror:\33[0m " fmt), \
		exit(EXIT_FAILURE))

int argc;
char** argv;

char* clname(int local_argc, char** local_argv) {
    argc = local_argc - 1;
    argv = local_argv + 1;
    return *local_argv;
}

int clflag() {
    if (!argc) return 0;
    if (**argv == '-')
    {
        argc--;
        return (*argv++)[1];
    }
    return -1;
}

char* clarg() {
    if (!argc--)
        panicf("expected an argument\n");
    return *argv++;
}

#endif