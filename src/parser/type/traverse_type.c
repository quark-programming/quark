#include "traverse_type.h"

#include "types.h"

static int traverse_generics(Declaration* const declaration, int (*acceptor)(Type*, Type*, void*), void* accumulator,
                             const unsigned flags, const u8 generics_offset) {
    if(!(flags & TraverseGenerics) || !len(declaration->generics.type_arguments_stacks[generics_offset])) return 0;

    Vec(Type*) const generics = last(declaration->generics.type_arguments_stacks[generics_offset]);
    for(size_t i = 0; i < len(generics); i++) {
        const int result = traverse_type(generics[i], NULL, acceptor, accumulator, flags, generics_offset);
        if(result) return result;
    }

    return 0;
}

// TODO: can be rewritten thanks to generics_offset
static int try_compare_same_declarations(Declaration* type_declaration, Vec(Action) const type_actions,
                                         Declaration* follower_declaration, Vec(Action) const follower_actions,
                                         int (*acceptor)(Type*, Type*, void*), void* accumulator,
                                         const unsigned flags) {
    if(type_declaration != follower_declaration) return 0;

    Vec(Type*) type_generics = find_last_generic_action(type_actions, type_declaration);
    Vec(Type*) follower_generics = find_last_generic_action(follower_actions, follower_declaration);

    // TODO: may be incorrect
    if(!len(type_generics) && type_declaration->generics.base_type_arguments) {
        type_generics = type_declaration->generics.type_arguments_stacks[0][0];
    }
    if(!len(follower_generics) && follower_declaration->generics.base_type_arguments) {
        follower_generics = follower_declaration->generics.type_arguments_stacks[1][0];
    }

    for(size_t i = 0; i < len(type_generics); i++) {
        const int result = traverse_type(type_generics[i], follower_generics[i], acceptor, accumulator, flags, 0);
        if(result) return result;
    }

    return 0;
}

int traverse_type(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*), void* accumulator,
                  const unsigned flags, const u8 generics_offset) {
    const OpenedType open_type =
            open_type_with_acceptor(type, follower, flags & TraverseIntermediate ? acceptor : 0, accumulator,
                                    flags & (ActionKeepGlobalState | ActionNoChildCompilation), generics_offset);
    const OpenedType open_follower = open_type(follower, flags & (ActionKeepGlobalState | ActionNoChildCompilation), 1);

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
                                       accumulator, flags, generics_offset);
                break;

            case NodeStructType:
                if(open_follower.type) {
                    result = try_compare_same_declarations((void*) open_type.type->StructType.module->declaration,
                                                           open_type.actions,
                                                           (void*) open_follower.type->StructType.module->declaration,
                                                           open_follower.actions, acceptor, accumulator, flags);
                    if(result) break;
                }

                result = traverse_generics((void*) open_type.type->StructType.module->declaration, acceptor,
                                           accumulator, flags, generics_offset);
                if(result) break;

                if(open_follower.type
                   && len(open_follower.type->StructType.fields) != len(open_type.type->StructType.fields)) {
                    result = 1;
                    break;
                }

                for(size_t i = 0; !result && i < len(open_type.type->StructType.fields); i++) {
                    result = traverse_type(open_type.type->StructType.fields[i].type,
                                           open_follower.type
                                               ? open_follower.type->StructType.fields[i].type
                                               : NULL,
                                           acceptor, accumulator, flags, generics_offset);
                }

                break;

            case NodeFunctionType:
                result = traverse_generics((void*) open_type.type->FunctionType.declaration, acceptor, accumulator,
                                           flags, generics_offset);
                break;

            default: ;
        }
    }

    close_type(open_type.actions, flags & (ActionKeepGlobalState | ActionNoChildCompilation), generics_offset);
    close_type(open_follower.actions, flags & (ActionKeepGlobalState | ActionNoChildCompilation), 1);
    return result - result_offset;
}
