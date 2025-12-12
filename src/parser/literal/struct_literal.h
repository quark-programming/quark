#ifndef STRUCT_LITERAL_H
#define STRUCT_LITERAL_H

#include "../nodes.h"

typedef struct StructLiteral {
    NODE_FIELDS;
    NodeVector field_values;
    StringVector field_names;
} StructLiteral;

#endif