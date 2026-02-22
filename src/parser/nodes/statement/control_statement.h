#ifndef NODE_CONTROL_STATEMENT_H
#define NODE_CONTROL_STATEMENT_H

#include "../fields.h"

typedef struct ControlStatement {
    NODE_FIELDS;
    str keyword;
    Vec(Node*) conditions;
    Scope* body;
} ControlStatement;

#endif