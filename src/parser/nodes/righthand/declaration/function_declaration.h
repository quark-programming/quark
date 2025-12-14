#ifndef NODE_FUNCTION_DECLARATION_H
#define NODE_FUNCTION_DECLARATION_H

#include "../../fields.h"

typedef struct Argument {
    Type* type;
    String identifier;
} Argument;

typedef Vector(Argument) ArgumentVector;

typedef struct FunctionDeclaration {
    DECLARATION_FIELDS;
    ArgumentVector arguments;
    struct Scope* body;
} FunctionDeclaration;

#endif