#ifndef FUNCTION_CALL_H
#define FUNCTION_CALL_H

#include "../nodes.h"

typedef struct FunctionCall {
    NODE_FIELDS;
    Node* function;
    NodeVector arguments;
} FunctionCall;

#endif