#ifndef NODE_IDENTIFIER_H
#define NODE_IDENTIFIER_H

#include "../../fields.h"
#include "parser/nodes/type/struct_type.h"

typedef struct Identifier {
    str base;
    Declaration* parent_scope;
    Declaration* parent_declaration;
    StructType* reference_structure;
    bool is_external;
} Identifier;

#endif