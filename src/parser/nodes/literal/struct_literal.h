#ifndef NODE_STRUCT_LITERAL_H
#define NODE_STRUCT_LITERAL_H

#include "../fields.h"

typedef struct StructLiteral {
    NODE_FIELDS;
    Vec(Node*) field_values;
    Vec(str) field_names;
} StructLiteral;

#endif