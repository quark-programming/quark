#include "types.h"

#include "stringify_type.h"
#include "../compiler/compiler.h"

bool global_in_compiler_step = false;
Compiler* global_compiler_context = NULL;
Vec(Action) global_actions = { 0 };

Type* new_type(Type type) {
    type.flags |= fType;
    Type* box = (void*) new_node((Node) { .Type = type });
    box->type = box;
    return box;
}

// TODO: possibly move actions to their own file
// TODO: remove return value if useless
bool apply_action(const Action action, const unsigned flags) {
    switch(action.type) {
        case ActionNone: return false;

        case ActionApplyGenerics: {
            push(&action.target->generics.type_arguments_stack, action.generics);

            if(!(flags & ActionKeepGlobalState)) {
                push(&global_actions, action);
            }

            static bool recursion_stop = false;
            if(global_in_compiler_step && !recursion_stop && !(flags & ActionNoChildCompilation)) {
                recursion_stop = true;

                String unique_key = NULL;
                stringify_generics(&unique_key, action.generics, StringifyAlphaNumeric);

                recursion_stop = false;

                if(!get(action.target->generics.unique_combinations, as_str(unique_key))) {
                    put(&action.target->generics.unique_combinations, as_str(unique_key));
                    compile(action.target, NULL, global_compiler_context);
                }
            }

            break;
        }

        case ActionApplyCollection:
            for(size_t i = 0; i < len(action.collection); i++) {
                apply_action(action.collection[i], flags);
            }
            break;

        default: unreachable();
    }

    return true;
}

void remove_action(const Action action, const unsigned flags) {
    switch(action.type) {
        case ActionNone: return;

        case ActionApplyGenerics:
            pop(&action.target->generics.type_arguments_stack);

            if(!(flags & ActionKeepGlobalState)) {
                pop(&global_actions);
            }

            break;

        case ActionApplyCollection:
            for(size_t i = len(action.collection); i > 0; i--) {
                remove_action(action.collection[i - 1], flags);
            }
            break;

        default: unreachable();
    }
}


Type* peek_type(Type* type, Action* action, const unsigned flags) {
    if(type->id == WrapperAuto || type->id == WrapperVariable || type->id == WrapperSurround) {
        if(apply_action(type->Wrapper.action, flags)) {
            *action = type->Wrapper.action;
        }
    }

    switch(type->id) {
        case WrapperAuto:
            if(!type->Wrapper.Auto.ref && type->Wrapper.Auto.test_against
               && type->Wrapper.Auto.test_against->id == NodeGenericReference) {
                GenericReference* const reference = &type->Wrapper.Auto.test_against->GenericReference;
                Vec(Vec(Type*)) const stack = reference->generics_declaration->generics.type_arguments_stack;

                for(size_t i = len(stack) - 1; i > 0; i--) {
                    Type* const type_argument = stack[i - 1][reference->index];

                    if(type_argument->id == WrapperAuto && type_argument->Wrapper.Auto.test_against
                       && type_argument->Wrapper.Auto.test_against->id == NodeGenericReference
                       && type_argument->Wrapper.Auto.test_against->GenericReference.generics_declaration
                       == reference->generics_declaration)
                        continue;

                    return type_argument;
                }
            }

            return type->Wrapper.Auto.ref ? type->Wrapper.Auto.ref : type;

        case WrapperVariable:
            return (void*) type->Wrapper.Variable.declaration->const_value;

        case NodeGenericReference:
            return last(type->GenericReference.generics_declaration->generics.type_arguments_stack)
                    [type->GenericReference.index];

        default: return type;
    }
}

OpenedType open_type_with_acceptor(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*),
                                   void* accumulator, const unsigned flags) {
    OpenedType opened_type = { 0 };
    if(!type) return opened_type;
    Action action = { 0 };

    while((opened_type.type = peek_type(type, &action, flags)) != type) {
        type = opened_type.type;
        if(acceptor) acceptor(type, follower, accumulator);

        if(action.type) {
            push(&opened_type.actions, action);
            action.type = 0;
        }
    }

    return opened_type;
}

void close_type(Vec(Action) const actions, const unsigned flags) {
    for(size_t i = len(actions); i > 0; i--) {
        remove_action(actions[i - 1], flags);
    }
    free(vbase(actions));
}

Vec(Type*) find_last_generic_action(Vec(Action) const actions, Declaration* const declaration) {
    for(size_t i = len(actions); i > 0; i--) {
        switch(actions[i - 1].type) {
            case ActionApplyGenerics:
                if(actions[i - 1].target == (void*) declaration) {
                    return actions[i - 1].generics;
                }
                break;

            case ActionApplyCollection: {
                Vec(Type*) const found = find_last_generic_action(actions[i - 1].collection, declaration);
                if(len(found)) return found;
                break;
            }

            default: unreachable();
        }
    }

    return NULL;
}

Type* make_type_standalone(Type* type) {
    if(!len(global_actions)) return type;

    Vec(Action) actions = NULL;
    resv(&actions, len(global_actions));
    memcpy(actions, global_actions, len(global_actions) * sizeof(Action));
    len(actions) = len(global_actions);

    return new_type((Type) {
        .Wrapper = {
            .id = WrapperAuto,
            .flags = type->flags,
            .trace = type->trace,
            .action = { ActionApplyCollection, .collection = actions },
            .Auto.ref = type,
        }
    });
}
