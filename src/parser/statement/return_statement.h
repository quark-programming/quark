#ifndef RETURN_STATEMENT_H
#define RETURN_STATEMENT_H

#include "../nodes.h"

typedef struct ReturnStatement {
    NODE_FIELDS;
    Node* value;
} ReturnStatement;

#endif