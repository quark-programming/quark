#include "clargs.h"

int argc;
const char** argv;

const char* clname(const int local_argc, const char** local_argv) {
    argc = local_argc - 1;
    argv = local_argv + 1;
    return *local_argv;
}

int clflag() {
    if(!argc) return 0;
    if(**argv == '-') {
        argc--;
        return (*argv++)[1];
    }
    return -1;
}

const char* clarg() {
    if(!argc--) {
        panicf("expected an argument\n");
    }
    return *argv++;
}
