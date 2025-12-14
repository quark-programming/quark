#ifndef NODE_STRUCT_TYPE_H
#define NODE_STRUCT_TYPE_H

#include "../fields.h"

typedef struct StructField {
    Type* type;
    String identifier;
} StructField;

typedef struct StructType {
    TYPE_FIELDS;
    Vector(StructField) fields;
    struct Scope* static_body;
    struct VariableDeclaration* parent;
} StructType;

#endif