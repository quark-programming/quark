#ifndef POINTER_TYPE_H
#define POINTER_TYPE_H

#include "../nodes.h"

typedef struct PointerType {
    NODE_FIELDS;
    Type* base;
} PointerType;

#endif