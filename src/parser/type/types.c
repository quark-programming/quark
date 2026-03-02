#include "types.h"

#include "stringify_type.h"
#include "../compiler/compiler.h"
#include "parser/parser.h"
#include "parser/lefthand/lefthand.h"
#include "parser/righthand/righthand.h"

bool global_in_compiler_step = false;
Compiler* global_compiler_context = NULL;
Vec(Action) global_actions[3] = { 0 };

Type* new_type(Type type) {
    type.flags |= fType;
    Type* box = (void*) new_node((Node) { .Type = type });
    box->type = box;
    return box;
}

// TODO: possibly move actions to their own file
// TODO: remove return value if useless
bool apply_action(const Action action, const unsigned flags, const u8 generics_offset) {
    switch(action.type) {
        case ActionNone: return false;

        case ActionApplyGenerics: {
            push(&action.target->generics.type_arguments_stacks[generics_offset], action.generics);

            if(!(flags & ActionKeepGlobalState)) {
                push(&global_actions[generics_offset], action);
            }

            // static bool recursion_stop = false;
            // if(global_in_compiler_step && !recursion_stop && !(flags & ActionNoChildCompilation)) {
            //     recursion_stop = true;
            //
            //     String unique_key = NULL;
            //     stringify_generics(&unique_key, action.generics, StringifyAlphaNumeric, generics_offset);
            //
            //     recursion_stop = false;
            //
            //     if(!get(action.target->generics.unique_combinations, as_str(unique_key))) {
            //         put(&action.target->generics.unique_combinations, as_str(unique_key));
            //         compile(action.target, NULL, global_compiler_context);
            //     }
            // }

            break;
        }

        case ActionApplyCollection:
            for(size_t i = 0; i < len(action.collection); i++) {
                apply_action(action.collection[i], flags, generics_offset);
            }
            break;

        case ActionAnchorTrait:
            if(!global_in_compiler_step || len(action.target->type->FunctionType.signature) < 2) break;
            Type* self_type = action.target->type->FunctionType.signature[1];

            if(self_type->id == WrapperAuto && self_type->Wrapper.Auto.constant) {
                self_type->Wrapper.Auto.ref = (void*) action.trait_struct;
            } else if(self_type->id == NodePointerType && self_type->PointerType.base->id == WrapperAuto
                      && self_type->PointerType.base->Wrapper.Auto.constant) {
                self_type->PointerType.base->Wrapper.Auto.ref = (void*) action.trait_struct;
            }

            break;

        default: unreachable();
    }

    return true;
}

void remove_action(const Action action, const unsigned flags, const u8 generics_offset) {
    switch(action.type) {
        case ActionNone: return;

        case ActionApplyGenerics:
            pop(&action.target->generics.type_arguments_stacks[generics_offset]);

            if(!(flags & ActionKeepGlobalState)) {
                pop(&global_actions[generics_offset]);
            }

            break;

        case ActionApplyCollection:
            for(size_t i = len(action.collection); i > 0; i--) {
                remove_action(action.collection[i - 1], flags, generics_offset);
            }
            break;

        case ActionAnchorTrait: break;

        default: unreachable();
    }
}

static bool is_stack_reference(Type* const type_argument, Declaration* const declaration) {
    return type_argument->id == WrapperAuto && type_argument->Wrapper.Auto.test_against
           && type_argument->Wrapper.Auto.test_against->id == NodeGenericReference
           && type_argument->Wrapper.Auto.test_against->GenericReference.generics_declaration
           == declaration;
}

Type* peek_type(Type* type, Action* action, const unsigned flags, u8 generics_offset) {
    if(!type) return type;

    if(type->id == WrapperAuto || type->id == WrapperVariable || type->id == WrapperSurround) {
        if(apply_action(type->Wrapper.action, flags, generics_offset)) {
            *action = type->Wrapper.action;
        }
    }

    switch(type->id) {
        case WrapperAuto:
            if(!type->Wrapper.Auto.ref && type->Wrapper.Auto.test_against
               && type->Wrapper.Auto.test_against->id == NodeGenericReference) {
                GenericReference* const reference = &type->Wrapper.Auto.test_against->GenericReference;

                do {
                    Vec(Vec(Type*)) stack =
                        reference->generics_declaration->generics.type_arguments_stacks[generics_offset];

                    for(size_t i = len(stack); i > 0; i--) {
                        Type* const type_argument = stack[i - 1][reference->index];
                        if(is_stack_reference(type_argument, reference->generics_declaration))
                            continue;
                        return type_argument;
                    }
                } while(generics_offset != 2 && ((generics_offset = 2)));

                panicf("path should not be reached");
            }

            return type->Wrapper.Auto.ref ? type->Wrapper.Auto.ref : type;

        case WrapperVariable:
            return (void*) type->Wrapper.Variable.declaration->const_value;

        case NodeGenericReference: {
            const Generics generics = type->GenericReference.generics_declaration->generics;
            const u8 normal_offset = len(generics.type_arguments_stacks[generics_offset]) ? generics_offset : 2;
            return last(generics.type_arguments_stacks[normal_offset])[type->GenericReference.index];
        }

        default: return type;
    }
}

OpenedType open_type_with_acceptor(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*),
                                   void* accumulator, const unsigned flags, const u8 generics_offset) {
    OpenedType opened_type = { 0 };
    if(!type) return opened_type;
    Action action = { 0 };

    while((opened_type.type = peek_type(type, &action, flags, generics_offset)) != type) {
        type = opened_type.type;
        if(acceptor) acceptor(type, follower, accumulator);

        if(action.type) {
            push(&opened_type.actions, action);
            action.type = 0;
        }
    }

    return opened_type;
}

void close_type(Vec(Action) const actions, const unsigned flags, const u8 generics_offset) {
    for(size_t i = len(actions); i > 0; i--) {
        remove_action(actions[i - 1], flags, generics_offset);
    }
    free(vbase(actions));
}

// TODO: Could be changed to just used declaration and a generics_offset
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

Type* make_type_standalone(Type* type, const u8 generics_offset) {
    if(!len(global_actions[generics_offset]) && !len(global_actions[2])) return type;

    Vec(Action) actions = NULL;
    const u32 length = len(global_actions[2]) + (generics_offset != 2) * len(global_actions[generics_offset]);
    resv(&actions, length);
    memcpy(actions, global_actions[2], len(global_actions[2]) * sizeof(Action));
    if(generics_offset != 2) {
        memcpy(actions + len(global_actions[2]), global_actions[generics_offset],
               len(global_actions[generics_offset]) * sizeof(Action));
    }
    len(actions) = length;

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

Type* get_type(Parser* parser) {
    Type* const type = (void*) righthand_expression(lefthand_expression(parser), parser, 11);
    if(!(type->flags & fType)) {
        push(parser->tokenizer->messages, MERROR(type->trace, str("expected a type here")));
        return type->type;
    }
    return type;
}
