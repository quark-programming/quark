#ifndef NODE_MODULE_H
#define NODE_MODULE_H

#include "../fields.h"

typedef struct Module {
    NODE_FIELDS;
    Declaration* declaration;
    Scope* scope;
    bool root;
} Module;

#endif