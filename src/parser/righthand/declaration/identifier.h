#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include "../../nodes.h"

typedef struct Identifier {
    String base;
    Declaration* parent_scope;
    Declaration* parent_declaration;
} Identifier;

#endif