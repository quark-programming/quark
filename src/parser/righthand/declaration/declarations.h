#ifndef DECLARATIONS_H
#define DECLARATIONS_H

#include "../../nodes.h"

#define DECLARATION_FIELDS \
    NODE_FIELDS; \
    struct Identifier* identifier; \
    union Node* const_value; \
    struct Generics generics; \
    bool is_inline : 1, \
         observerd : 1

#endif