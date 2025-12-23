#ifndef TYPES_H
#define TYPES_H

#include <vector.h>

#include "../nodes/nodes.h"

Type* new_type(Type type);

typedef struct OpenedType {
    ActionVector actions;
    Type* type;
} OpenedType;

extern bool global_in_compiler_step;
extern Compiler* global_compiler_context;
extern ActionVector global_actions;

enum {
    TraverseIntermediate     = 1 << 0,
    TraverseGenerics         = 1 << 1,
    ActionKeepGlobalState    = 1 << 2,
    ActionNoChildCompilation = 1 << 3,
};

bool apply_action(Action action, unsigned flags);

void remove_action(Action action, unsigned flags);

Type* peek_type(Type* type, Action* action, unsigned flags);

OpenedType open_type_with_acceptor(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*),
                                   void* accumulator, unsigned flags);

#define open_type(type, flags) open_type_with_acceptor(type, NULL, NULL, NULL, flags);

void close_type(ActionVector actions, unsigned flags);

TypeVector find_last_generic_action(ActionVector actions, Declaration* declaration);

Type* make_type_standalone(Type* type);

int traverse_type(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*), void* accumulator,
                  unsigned flags);

#endif
