#include "clargs.h"

int clflag(int* argc, char*** argv) {
    if(!--*argc) return 0;
    return *(*argv)[1] == '-' ? (*++*argv)[1] : -1;
}

char* clarg(int* argc, char*** argv) {
    if(***argv == '-' && (**argv)[2]) return **argv + 2;
    if(!(*argc -= ***argv == '-')) panicf("expected an argument\n");
    return *++*argv;
}