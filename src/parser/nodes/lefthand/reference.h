#ifndef NODE_REFERENCE_H
#define NODE_REFERENCE_H

#include "../fields.h"

typedef struct Reference {
    NODE_FIELDS;
    Type* dereferenced_type;
    Node* value;
} Reference;

#endif