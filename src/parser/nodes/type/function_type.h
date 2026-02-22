#ifndef NODE_FUNCTION_TYPE_H
#define NODE_FUNCTION_TYPE_H

#include "../fields.h"

typedef struct FunctionType {
    TYPE_FIELDS;
    Vec(Type*) signature;
    struct FunctionDeclaration* declaration;
    Map(bool) type_definitions;
} FunctionType;

#endif