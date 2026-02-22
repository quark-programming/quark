#ifndef NODE_NUMERIC_LITERAL_H
#define NODE_NUMERIC_LITERAL_H

#include "../fields.h"

typedef struct NumericLiteral {
    NODE_FIELDS;
    i64 value;
} NumericLiteral;

#endif