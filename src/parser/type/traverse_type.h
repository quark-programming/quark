#ifndef TRAVERSE_TYPES_H
#define TRAVERSE_TYPES_H

#include "../parser.h"

int traverse_type(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*), void* accumulator,
                  unsigned flags, u8 generics_offset);

int traverse_action(Action action, int (*acceptor)(Type*, Type*, void*), void* accumulator, unsigned flags,
                    u8 generics_offset);

#endif
