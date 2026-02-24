#ifndef NODE_SCOPE_H
#define NODE_SCOPE_H

#include "../fields.h"

typedef struct Scope {
    NODE_FIELDS;
    Vec(Node*) children;
    Map(Declaration*) variables;
    Declaration* declaration;
    Node* result_value;
    Vec(struct Scope*) wildcards;
    bool wrap_with_brackets;
} Scope;

typedef Vec(Scope*) Stack;

#endif