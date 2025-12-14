#ifndef NODE_POINTER_TYPE_H
#define NODE_POINTER_TYPE_H

#include "../fields.h"

typedef struct PointerType {
    NODE_FIELDS;
    Type* base;
} PointerType;

#endif