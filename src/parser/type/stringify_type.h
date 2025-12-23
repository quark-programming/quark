#ifndef STRINGIFY_TYPE_H
#define STRINGIFY_TYPE_H

#include "../nodes/nodes.h"

enum {
    StringifyAlphaNumeric = 1 << 0,
};

typedef struct {
    String* string;
    unsigned flags;
} StringifyAccumulator;

void stringify_type(Type* type, String* string, unsigned flags);

void stringify_generics(String* string, TypeVector generics, unsigned flags);

#endif