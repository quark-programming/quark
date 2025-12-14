#ifndef RIGHTHAND_H
#define RIGHTHAND_H

#include "../nodes/nodes.h"
#include "../parser.h"

extern bool global_righthand_collecting_type_arguments;

Node* expression(Parser* parser);

#endif
