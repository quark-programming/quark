#ifndef NODE_IDENTIFIER_H
#define NODE_IDENTIFIER_H

#include "../../fields.h"

typedef struct Identifier {
    str base;
    struct Scope* parent_scope;
    Declaration* parent_declaration;
    struct StructDeclaration* trait;
    bool is_external;
} Identifier;

#endif