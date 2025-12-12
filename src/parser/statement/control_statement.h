#ifndef CONTROL_STATEMENT_H
#define CONTROL_STATEMENT_H

#include "../nodes.h"

typedef struct ControlStatement {
    NODE_FIELDS;
    String keyword;
    NodeVector conditions;
    struct Scope* body;
} ControlStatement;

#endif