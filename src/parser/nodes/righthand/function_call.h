#ifndef NODE_FUNCTION_CALL_H
#define NODE_FUNCTION_CALL_H

#include "../fields.h"

typedef struct FunctionCall {
    NODE_FIELDS;
    Node* function;
    Vec(Node*) arguments;
} FunctionCall;

#endif