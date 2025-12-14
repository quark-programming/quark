#ifndef NODE_FUNCTION_TYPE_H
#define NODE_FUNCTION_TYPE_H

#include "../fields.h"

typedef struct FunctionType {
    TYPE_FIELDS;
    TypeVector signature;
    struct FunctionDeclaration* declaration;
} FunctionType;

#endif