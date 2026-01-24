#ifndef NODE_FUNCTION_DECLARATION_H
#define NODE_FUNCTION_DECLARATION_H

#include "../../fields.h"

typedef struct Argument {
    Type* type;
    String identifier;
} Argument;

typedef Vector(Argument) ArgumentVector;

typedef Vector(VariableDeclaration*) VariableDeclarationVector;

typedef struct FunctionDeclaration {
    DECLARATION_FIELDS;
    ArgumentVector arguments;
    struct Scope* body;
    VariableDeclarationVector variable_declarations;
} FunctionDeclaration;

#endif