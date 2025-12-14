#ifndef NODE_STRUCT_LITERAL_H
#define NODE_STRUCT_LITERAL_H

#include "../fields.h"

typedef struct StructLiteral {
    NODE_FIELDS;
    NodeVector field_values;
    StringVector field_names;
} StructLiteral;

#endif