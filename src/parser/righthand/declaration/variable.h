#ifndef VARIABLE_DECLARATION_H
#define VARIABLE_DECLARATION_H

#include "../../nodes/nodes.h"
#include "../../parser.h"
#include "identifier.h"

Node* parse_variable_declaration(Type* type, IdentifierInfo info, Parser* parser);

Node* create_temp_variable(Node* value, Parser* parser, Vec(Node*)* collector);

#endif