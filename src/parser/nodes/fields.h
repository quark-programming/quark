#ifndef FIELDS_H
#define FIELDS_H

#include <vector.h>

#include "../../tokenizer/tokenizer.h"

#define NODE_FIELDS \
    uint32_t id; \
    uint32_t flags; \
    Trace trace; \
    Type* type

#define TYPE_FIELDS \
    NODE_FIELDS

#define DECLARATION_FIELDS \
    NODE_FIELDS; \
    struct Identifier identifier; \
    union Node* const_value; \
    struct Generics generics; \
    bool is_inline : 1, \
         observerd : 1

typedef struct Compiler Compiler;
typedef union Node Node;
typedef union Type Type;
typedef union Declaration Declaration;

typedef Vector(Node*) NodeVector;
typedef Vector(Type*) TypeVector;
typedef Vector(Declaration*) DeclarationVector;

#endif
