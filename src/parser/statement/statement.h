#ifndef STATEMENT_H
#define STATEMENT_H

#include "../nodes/nodes.h"
#include "../parser.h"

Vec(Node*) collect_until(Parser* parser, Node* (*supplier)(Parser*), char separator, char terminator);

void collect_into(Parser* parser, Node* (*supplier)(Parser*), char separator, char terminator, Vec(Node*)* collector);

Node* statement(Parser* parser);

extern Vec(str) global_library_paths;

#endif
