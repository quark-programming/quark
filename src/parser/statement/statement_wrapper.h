#ifndef STATEMENT_WRAPPER_H
#define STATEMENT_WRAPPER_H

#include "../nodes.h"

typedef struct StatementWrapper {
    NODE_FIELDS;
    Node* expression;
} StatementWrapper;

#endif