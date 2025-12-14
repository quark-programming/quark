#ifndef STATEMENT_H
#define STATEMENT_H

#include "../nodes/nodes.h"
#include "../parser.h"

NodeVector collect_until(Parser* parser, Node* (*supplier)(Parser*), char separator, char terminator);

#endif
