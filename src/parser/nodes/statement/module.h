#ifndef NODE_MODULE_H
#define NODE_MODULE_H

#include "../fields.h"

typedef struct Module {
    DECLARATION_FIELDS;
    Declaration* declaration;
    Scope* scope;
} Module;

#endif