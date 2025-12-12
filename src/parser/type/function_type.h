#ifndef FUNCTION_TYPE_H
#define FUNCTION_TYPE_H

#include "../nodes.h"

typedef struct FunctionType {
    TYPE_FIELDS;
    TypeVector signature;
    struct FunctionDeclaration* declaration;
} FunctionType;

#endif