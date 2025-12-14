#ifndef NODE_RETURN_STATEMENT_H
#define NODE_RETURN_STATEMENT_H

#include "../fields.h"

typedef struct ReturnStatement {
    NODE_FIELDS;
    Node* value;
} ReturnStatement;

#endif