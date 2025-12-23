#include "types.h"
#include "stringify_type.h"

Type* new_type(Type type) {
    type.flags |= fType;
    Type* box = (void*) new_node((Node) { .Type = type });
    box->type = box;
    return box;
}

// TODO: possibly move actions to their own file
bool apply_action(const Action action, const unsigned flags) {
    switch(action.type) {
        case ActionApplyGenerics: {
            for(size_t i = 0; i < action.generics.size; i++) {
                if(action.generics.data[i]->id == WrapperAuto
                   && action.generics.data[i]->Wrapper.Auto.replacement_generic
                   && action.generics.data[i]->Wrapper.Auto.replacement_generic->generics_declaration
                   == action.target) {
                    return false;
                }
            }

            push(&action.target->generics.type_arguments_stack, action.generics);

            if(!(flags & ActionKeepGlobalState)) {
                push(&global_actions, action);
            }

            static bool recursion_stop = false;
            if(global_in_compiler_step && !recursion_stop && !(flags & ActionNoChildCompilation)) {
                recursion_stop = true;

                String unique_key = { 0 };
                stringify_generics(&unique_key, action.generics, StringifyAlphaNumeric);

                recursion_stop = false;

                if(!get(action.target->generics.unique_combinations, unique_key)) {
                    put(&action.target->generics.unique_combinations, unique_key);
                    action.target->generics.monomorphic_compiler(action.target, NULL, global_compiler_context);
                }
            }

            break;
        }

        case ActionApplyCollection:
            for(size_t i = 0; i < action.collection.size; i++) {
                apply_action(action.collection.data[i], flags);
            }
            break;

        default: unreachable();
    }

    return true;
}

void remove_action(Action action, const unsigned flags) {
    switch(action.type) {
        case ActionApplyGenerics:
            pop(&action.target->generics.type_arguments_stack);

            if(!(flags & ActionKeepGlobalState)) {
                pop(&global_actions);
            }

            break;

        case ActionApplyCollection:
            for(size_t i = action.collection.size; i > 0; i--) {
                remove_action(action.collection.data[i - 1], flags);
            }
            break;

        default: unreachable();
    }
}


Type* peek_type(Type* type, Action* action, const unsigned flags) {
    switch(type->id) {
        case WrapperAuto:
            if(global_in_compiler_step && type->Wrapper.Auto.replacement_generic) {
                return peek_type((void*) type->Wrapper.Auto.replacement_generic, action, flags);
            }

            if(!type->Wrapper.child) return type;

            if(apply_action(type->Wrapper.action, flags)) {
                *action = type->Wrapper.action;
            }

            return type->Wrapper.Auto.ref;

        case WrapperVariable:
            return (void*) type->Wrapper.Variable.declaration->const_value;

        case NodeGenericReference:
            return last(type->GenericReference.generics_declaration->generics.type_arguments_stack)
                    .data[type->GenericReference.index];

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

void close_type(const ActionVector actions, const unsigned flags) {
    for(size_t i = actions.size; i > 0; i--) {
        remove_action(actions.data[i - 1], flags);
    }
    free(actions.data);
}

TypeVector find_last_generic_action(const ActionVector actions, Declaration* const declaration) {
    for(size_t i = actions.size; i > 0; i--) {
        switch(actions.data[i - 1].type) {
            case ActionApplyGenerics:
                if(actions.data[i - 1].target == (void*) declaration) {
                    return actions.data[i - 1].generics;
                }
                break;

            case ActionApplyCollection: {
                const TypeVector found = find_last_generic_action(actions.data[i - 1].collection, declaration);
                if(found.size) return found;
                break;
            }

            default: unreachable();
        }
    }

    return (TypeVector) { 0 };
}

Type* make_type_standalone(Type* type) {
    if(!global_actions.size) return type;

    ActionVector actions = { 0 };
    resv(&actions, global_actions.size);
    memcpy(actions.data, global_actions.data, global_actions.size * sizeof(Action));

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

static int traverse_generics(Declaration* const declaration, int (*acceptor)(Type*, Type*, void*), void* accumulator,
                             unsigned flags) {
    if(!(flags & TraverseGenerics) || !declaration->generics.type_arguments_stack.size) return 0;

    const TypeVector generics = last(declaration->generics.type_arguments_stack);
    for(size_t i = 0; i < generics.size; i++) {
        const int result = traverse_type(generics.data[i], NULL, acceptor, accumulator,
                                         flags);
        if(result) return result;
    }

    return 0;
}

static int try_compare_same_declarations(Declaration* type_declaration, const ActionVector type_actions,
                                         Declaration* follower_declaration, const ActionVector follower_actions,
                                         int (*acceptor)(Type*, Type*, void*), void* accumulator,
                                         const unsigned flags) {
    if(type_declaration != follower_declaration) return 0;

    TypeVector type_generics = find_last_generic_action(type_actions, type_declaration);
    TypeVector follower_generics = find_last_generic_action(follower_actions, follower_declaration);

    // TODO: may be incorrect
    if(!type_generics.size) {
        type_generics = type_declaration->generics.type_arguments_stack.data[0];
    }
    if(!follower_generics.size) {
        follower_generics = follower_declaration->generics.type_arguments_stack.data[0];
    }

    for(size_t i = 0; i < type_generics.size; i++) {
        const int result = traverse_type(type_generics.data[i], follower_generics.data[i], acceptor, accumulator,
                                         flags);
        if(result) return result;
    }

    return 0;
}

int traverse_type(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*), void* accumulator,
                  const unsigned flags) {
    const OpenedType open_follower = open_type(follower, flags & (ActionKeepGlobalState | ActionNoChildCompilation));
    const OpenedType open_type =
            open_type_with_acceptor(type, follower, flags & TraverseIntermediate ? acceptor : 0, accumulator,
                                    flags & (ActionKeepGlobalState | ActionNoChildCompilation));

    int result = 0, result_offset = 0;
    if(!(flags & TraverseIntermediate)) {
        result_offset = !!((result = acceptor(open_type.type, open_follower.type, accumulator)));
    }

    if(result) {
    } else if(follower && open_type.type->id != open_follower.type->id) {
        result = 1;
    } else {
        switch(open_type.type->id) {
            case NodePointerType:
                result = traverse_type(open_type.type->PointerType.base,
                                       open_follower.type ? open_follower.type->PointerType.base : NULL, acceptor,
                                       accumulator, flags);
                break;

            case NodeStructType:
                if(open_follower.type) {
                    result = try_compare_same_declarations((void*) open_type.type->StructType.parent, open_type.actions,
                                                           (void*) open_follower.type->StructType.parent,
                                                           open_follower.actions, acceptor, accumulator, flags);
                    if(result) break;
                }

                result = traverse_generics((void*) open_type.type->StructType.parent, acceptor, accumulator,
                                           flags);
                if(result) break;

                if(open_follower.type && open_follower.type->StructType.fields.size != open_type.type->StructType.fields
                   .size) {
                    result = 1;
                    break;
                }

                for(size_t i = 0; !result && i < open_type.type->StructType.fields.size; i++) {
                    result = traverse_type(open_type.type->StructType.fields.data[i].type,
                                           open_follower.type
                                               ? open_follower.type->StructType.fields.data[i].type
                                               : 0,
                                           acceptor, accumulator, flags);
                }

                break;

            case NodeFunctionType:
                result = traverse_generics((void*) open_type.type->FunctionType.declaration, acceptor, accumulator,
                                           flags);
                break;

            default: ;
        }
    }

    close_type(open_type.actions, flags & (ActionKeepGlobalState | ActionNoChildCompilation));
    close_type(open_follower.actions, flags & (ActionKeepGlobalState | ActionNoChildCompilation));
    return result - result_offset;
}
