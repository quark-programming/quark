#ifndef NODE_FUNCTION_DECLARATION_H
#define NODE_FUNCTION_DECLARATION_H

#include "../../fields.h"

typedef struct Argument {
    Type* type;
    str identifier;
} Argument;

typedef struct FunctionDeclaration {
    DECLARATION_FIELDS;
    Vec(Argument) arguments;
    struct Scope* body;
    Vec(VariableDeclaration*) variable_declarations;
} FunctionDeclaration;

#endif