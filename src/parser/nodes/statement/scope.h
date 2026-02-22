#ifndef NODE_SCOPE_H
#define NODE_SCOPE_H

#include "../fields.h"

typedef struct Scope {
    NODE_FIELDS;
    Vec(Node*) children;
    Map(Declaration*) variables;
    // DeclarationVector hoisted_declarations;
    Declaration* parent;
    Node* result_value;
    bool wrap_with_brackets;
} Scope;

typedef Vec(Scope*) Stack;

#endif