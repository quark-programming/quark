#ifndef NODE_STRUCT_TYPE_H
#define NODE_STRUCT_TYPE_H

#include "../fields.h"
#include "../statement/scope.h"

typedef struct StructField {
    Type* type;
    str identifier;
} StructField;

typedef struct StructType {
    TYPE_FIELDS;
    Vec(StructField) fields;
    Scope* static_body;
    struct StructDeclaration* parent;
    Map(Scope) reference_structures;
} StructType;

#endif