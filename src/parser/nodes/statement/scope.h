#ifndef NODE_SCOPE_H
#define NODE_SCOPE_H

#include <hashmap.h>

#include "../fields.h"

typedef HashMap(Declaration*) DeclarationHashMap;

typedef struct Scope {
    NODE_FIELDS;
    NodeVector children;
    DeclarationHashMap variables;
    // DeclarationVector hoisted_declarations;
    Declaration* parent;
    Node* result_value;
    bool wrap_with_brackets : 1;
} Scope;

typedef Vector(Scope*) Stack;

#endif