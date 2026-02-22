#ifndef FIELDS_H
#define FIELDS_H

#include <helpers.h>
#include <wrap.h>
#include "../../tokenizer/tokenizer.h"

#define NODE_FIELDS \
    NodeID id; \
    u32 flags; \
    Trace trace; \
    Type* type

#define TYPE_FIELDS \
    NODE_FIELDS

#define DECLARATION_FIELDS \
    NODE_FIELDS; \
    struct Identifier identifier; \
    union Node* const_value; \
    struct Generics generics; \
    uint8_t compilation_state;

typedef struct Compiler Compiler;
typedef union Node Node;
typedef union Type Type;
typedef union Declaration Declaration;

#endif
