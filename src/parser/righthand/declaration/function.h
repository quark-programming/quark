#ifndef FUNCTION_DECLARATION_H
#define FUNCTION_DECLARATION_H

#include "../../nodes/nodes.h"
#include "../../parser.h"

#include "identifier.h"

Node* parse_function_declaration(Type* return_type, IdentifierInfo info, Parser* parser, bool no_body);

Node* parse_function_lambda(Type* return_type, Parser* parser);

#endif