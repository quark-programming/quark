#ifndef NODE_VARIABLE_DECLARATION_H
#define NODE_VARIABLE_DECLARATION_H

#include "../../fields.h"

typedef struct VariableDeclaration {
    DECLARATION_FIELDS;
    Node* static_value;
} VariableDeclaration;

#endif