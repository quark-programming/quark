#ifndef NODE_NUMERIC_LITERAL_H
#define NODE_NUMERIC_LITERAL_H

#include <stdint.h>

#include "../fields.h"

typedef struct NumericLiteral {
    NODE_FIELDS;
    int64_t value;
} NumericLiteral;

#endif