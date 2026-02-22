#ifndef NODE_BINARY_OPERATION_H
#define NODE_BINARY_OPERATION_H

#include "../fields.h"

typedef struct BinaryOperation {
    NODE_FIELDS;
    Node* left;
    str operator;
    Node* right;
} BinaryOperation;

#endif