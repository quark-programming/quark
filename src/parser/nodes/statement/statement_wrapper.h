#ifndef NODE_STATEMENT_WRAPPER_H
#define NODE_STATEMENT_WRAPPER_H

#include "../fields.h"

typedef struct StatementWrapper {
    NODE_FIELDS;
    Node* expression;
} StatementWrapper;

#include "../nodes.h"

#endif