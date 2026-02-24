#ifndef STD_H
#define STD_H

#include "parser/type/types.h"

extern Scope global_std_scope;

void build_std_scope(Declaration* declaration);

#endif