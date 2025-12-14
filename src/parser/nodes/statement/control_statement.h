#ifndef NODE_CONTROL_STATEMENT_H
#define NODE_CONTROL_STATEMENT_H

#include "../fields.h"

typedef struct ControlStatement {
    NODE_FIELDS;
    String keyword;
    NodeVector conditions;
    Scope* body;
} ControlStatement;

#endif