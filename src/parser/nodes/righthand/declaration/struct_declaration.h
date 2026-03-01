#ifndef NODE_STRUCT_DECLARATION_H
#define NODE_STRUCT_DECLARATION_H

#include "../../fields.h"

typedef struct StructDeclaration {
    DECLARATION_FIELDS;
    Vec(Declaration*) trait_declarations;
} StructDeclaration;

typedef struct TraitAccess {
    NODE_FIELDS;
    Type* generic_type;
    Declaration* trait_declaration;
    Trace field_trace;
    Node* bound_self_argument;
} TraitAccess;

#endif