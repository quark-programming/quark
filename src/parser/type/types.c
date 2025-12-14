#include "types.h"
#include "stringify_type.h"

Type* new_type(Type type) {
    type.flags |= fType;
    Type* box = (void*) new_node((Node) { .Type = type });
    box->type = box;
    return box;
}

bool apply_action(Action action, const unsigned flags) {
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


Type* peek_type(Type* type, Action* action, unsigned flags) {
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
    for (size_t i = actions.size; i > 0; i--) {
        remove_action(actions.data[i - 1], flags);
    }
    free(actions.data);
}
