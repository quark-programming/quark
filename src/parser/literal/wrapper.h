#ifndef WRAPPER_H
#define WRAPPER_H

#include "../nodes/nodes.h"

Vec(Action) extract_link_actions(Declaration** declaration, Vec(Action)* actions);

Wrapper* variable_of(Declaration* declaration, Trace trace, unsigned long flags);

#endif