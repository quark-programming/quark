#ifndef TYPES_H
#define TYPES_H

#include "../nodes/nodes.h"

Type* new_type(Type type);

typedef struct OpenedType {
    Vec(Action) actions;
    Type* type;
} OpenedType;

extern bool global_in_compiler_step;
extern Compiler* global_compiler_context;
extern Vec(Action) global_actions[3];

enum {
    TraverseIntermediate     = 1 << 0,
    TraverseGenerics         = 1 << 1,
    ActionKeepGlobalState    = 1 << 2,
    ActionNoChildCompilation = 1 << 3,
};

bool apply_action(Action action, unsigned flags, u8 generics_offset);

void remove_action(Action action, unsigned flags, u8 generics_offset);

extern Vec(Message)* global_critical_messages;

Type* peek_type(Type* type, Action* action, unsigned flags, u8 generics_offset);

OpenedType open_type_with_acceptor(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*),
                                   void* accumulator, unsigned flags, u8 generics_offset);

#define open_type(type, flags, generics_offset) open_type_with_acceptor(type, NULL, NULL, NULL, flags, generics_offset)

void close_type(Vec(Action) actions, unsigned flags, u8 generics_offset);

Vec(Type*) find_last_generic_action(Vec(Action) actions, Declaration* declaration);

Type* make_type_standalone(Type* type, u8 generics_offset);

Type* get_type(struct Parser* parser);

#endif
