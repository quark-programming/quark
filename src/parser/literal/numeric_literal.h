#ifndef NUMERIC_LITERAL_H
#define NUMERIC_LITERAL_H

#include <stdint.h>

#include "../nodes.h"

typedef struct NumericLiteral {
    NODE_FIELDS;
    int64_t value;
} NumericLiteral;

#endif