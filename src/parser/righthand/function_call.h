#ifndef RIGHTHAND_FUNCTION_CALL_H
#define RIGHTHAND_FUNCTION_CALL_H

#include "../parser.h"

Declaration* fetch_operator_override(Type* type, String override);

Node* operator_override_n(Type* type, Node* self, NodeVector arguments, const String override, const Trace trace,
                          Parser* parser);

Node* operator_override(Type* type, Node* self, Node* argument, const String override, const Trace trace,
                        Parser* parser);

Node* parse_function_call(Node* function, Parser* parser);

Node* call_function(Node* function, NodeVector arguments, Parser* parser);

#endif
