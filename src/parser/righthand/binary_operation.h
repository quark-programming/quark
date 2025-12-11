#ifndef BINARY_OPERATION_H
#define BINARY_OPERATION_H

#include <vector-string.h>

#include "../nodes.h"

typedef struct BinaryOperation {
    NODE_FIELDS;
    Node* left;
    String operator;
    Node* right;
} BinaryOperation;

#endif