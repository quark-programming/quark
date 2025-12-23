#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__has_builtin) && __has_builtin(__builtin_unreachable)
#define unreachable() __builtin_unreachable()
#else
#define unreachable() assert(0 && "Unreachable")
#endif

#if defined(__has_attribute) && __has_attribute(nonnull)
#define notnull __attribute__((nonnull))
#else
#define notnull
#endif

char* fs_readfile(notnull const char* filename);

typedef int UsableVoid[0];

#endif
