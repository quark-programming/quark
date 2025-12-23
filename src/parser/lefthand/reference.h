#ifndef REFERENCE_H
#define REFERENCE_H

#include "../nodes/nodes.h"
#include "../parser.h"

Node* reference(Node* node, Trace trace);

Node* dereference(Node* node, Trace trace, MessageVector* messages);

#endif