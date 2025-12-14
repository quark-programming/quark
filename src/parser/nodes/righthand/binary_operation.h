#ifndef NODE_BINARY_OPERATION_H
#define NODE_BINARY_OPERATION_H

#include <vector-string.h>

#include "../fields.h"

typedef struct BinaryOperation {
    NODE_FIELDS;
    Node* left;
    String operator;
    Node* right;
} BinaryOperation;

#endif