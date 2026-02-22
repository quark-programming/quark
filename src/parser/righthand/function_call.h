#ifndef RIGHTHAND_FUNCTION_CALL_H
#define RIGHTHAND_FUNCTION_CALL_H

#include "../parser.h"

Declaration* fetch_operator_override(Type* type, str override);

Node* operator_override_n(Type* type, Node* self, Vec(Node*) arguments, str override, Trace trace, Parser* parser);

Node* operator_override(Type* type, Node* self, Node* argument, str override, Trace trace, Parser* parser);

Node* parse_function_call(Node* function, Parser* parser);

Node* call_function(Node* function, Vec(Node*) arguments, Parser* parser);

#endif
