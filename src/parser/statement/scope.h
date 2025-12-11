#ifndef SCOPE_H
#define SCOPE_H

#include <hashmap.h>

#include "../nodes.h"

typedef struct Scope {
    NODE_FIELDS;
    HashMap(Declaration*) variables;
    DeclarationVector hoisted_declarations;
    Declaration* parent;
    Node* result_value;
    bool wrap_with_brackets : 1;
} Scope;

typedef Vector(Scope*) Stack;

#endif