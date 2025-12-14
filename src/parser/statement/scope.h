#ifndef SCOPE_H
#define SCOPE_H

#include "../nodes/nodes.h"
#include "../parser.h"

Scope* new_scope(Declaration* parent);

Wrapper* find_in_scope(Scope scope, Trace identifier);

Wrapper* find_on_stack(Stack stack, Trace identifier);

#endif