#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "../nodes/nodes.h"
#include "../parser.h"

Node* parse_struct_literal(Type* wrapper_struct_type, Parser* parser);

#endif