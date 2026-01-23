#ifndef NODE_STRUCT_TYPE_H
#define NODE_STRUCT_TYPE_H

#include <hashmap.h>

#include "../fields.h"
#include "parser/nodes/statement/scope.h"

typedef struct StructField {
    Type* type;
    String identifier;
} StructField;

typedef HashMap(Scope) ScopeHashMap;

typedef struct StructType {
    TYPE_FIELDS;
    Vector(StructField) fields;
    struct Scope* static_body;
    struct StructDeclaration* parent;
    ScopeHashMap reference_structures;
} StructType;

#endif