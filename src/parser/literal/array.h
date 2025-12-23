#ifndef LITERAL_ARRAY_H
#define LITERAL_ARRAY_H

#include "../nodes/nodes.h"
#include "../parser.h"

Node* parse_array_literal(Trace trace_start, Parser* parser);

#endif