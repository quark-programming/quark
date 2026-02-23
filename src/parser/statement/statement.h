#ifndef STATEMENT_H
#define STATEMENT_H

#include "../nodes/nodes.h"
#include "../parser.h"

Vec(Node*) collect_until(Parser* parser, Node* (*supplier)(Parser*), char separator, char terminator);

Node* statement(Parser* parser);

extern Vec(str) global_library_paths;

#endif
