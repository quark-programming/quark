#include "clash_types.h"
#include "types.h"
#include "stringify_type.h"
#include "traverse_type.h"
#include "tty.h"
#include "parser/literal/wrapper.h"
#include "parser/statement/scope.h"

static int circular_acceptor(Type* const type, Type* follower, void* const compare) {
    (void) follower;
    return (TestCircular - 1) * (type == compare);
}

static int clash_acceptor(Type* type, Type* follower, void* void_accumulator);

static Type** order_types(Type* left, Type* right, const bool swap, Type** result) {
    result[0] = swap ? right : left;
    result[1] = swap ? left : right;
    return result;
}

static int test_required_traits(Wrapper* left, StructType* right, ClashAccumulator* accumulator,
                                const u8 generics_offset) {
    if(!left->Auto.required_traits || right->id == WrapperAuto) return 0;
    if(right->id != NodeStructType) return 1;

    for(u32 i = 0; i < len(left->Auto.required_traits); i++) {
        // Match Only
        const OpenedType opened_trait = open_type(left->Auto.required_traits[i]->type, 0, !generics_offset);
        close_type(opened_trait.actions, 0, !generics_offset);

        switch(opened_trait.type->id) {
            case NodeFunctionType: {
                Scope* const trait_functions = get(right->traits, opened_trait.type->FunctionType.declaration
                                                   ->identifier.parent_scope->declaration->identifier.base);
                if(!trait_functions) return 1;

                Declaration* function_declaration =
                        find_in_scope_unwrapped(*trait_functions, opened_trait.type->FunctionType.declaration
                                                                              ->identifier.base);
                if(!function_declaration) return 1;

                const Action function_action = {
                    .type = ActionApplyCollection,
                    .collection = extract_link_actions(&function_declaration, NULL),
                };
                apply_action(function_action, 0, !generics_offset);

                Type** const types = order_types(left->Auto.required_traits[i]->type, function_declaration->type,
                                                 !generics_offset, (Type*[2]) {});
                const int result = traverse_type(types[0], types[1], &clash_acceptor, accumulator,
                                                 accumulator->flags, 0);

                remove_action(function_action, 0, !generics_offset);
                if(result) return result;
                break;
            }

            case NodeStructType: {
                Scope* const trait_functions = get(right->traits, opened_trait.type->StructType.module->declaration
                                                   ->identifier.base);
                if(!trait_functions) return 1;

                Type** const types = order_types(left->Auto.required_traits[i]->type,
                                                 (void*) trait_functions->result_value, !generics_offset,
                                                 (Type*[2]) {});
                const int result = traverse_type(types[0], types[1], &clash_acceptor, accumulator,
                                                 accumulator->flags, 0);
                if(result) return result;
                break;
            }

            default: ;
        }
    }

    return 0;
}

static void move_required_traits(Vec(Wrapper*)* dest, Vec(Wrapper*) src) {
    for(u32 i = 0, j; i < len(src); i++) {
        for(j = 0; j < len(*dest) && src[i] != (*dest)[i]; j++);
        if(j == len(*dest))
            push(dest, src[i]);
    }
}

static int assign_auto_ref(Type* type, Type* follower, ClashAccumulator* accumulator, const bool passive) {
    u8 generics_offset = 1;
    Wrapper* wrapper = &type->Wrapper;

    if(type->id != WrapperAuto
       || (follower->id == WrapperAuto && follower->Wrapper.Auto.priority > wrapper->Auto.priority)) {
        wrapper = &follower->Wrapper;
        follower = type;
        generics_offset = 0;
    }

    if(follower->id != WrapperAuto && wrapper->flags & fNumeric && !(follower->flags & fNumeric)) return TestMismatch;
    if(passive || wrapper->Auto.constant) return 1;
    const int result = test_required_traits(wrapper, (void*) follower, accumulator, generics_offset);
    if(result) return result;

    wrapper->Auto.ref = make_type_standalone(follower, generics_offset);

    if(follower->id == WrapperAuto) {
        if((wrapper->flags | follower->flags) & fNumeric) {
            wrapper->flags |= fNumeric;
            follower->flags |= fNumeric;
        }
        if(wrapper->Auto.test_against) follower->Wrapper.Auto.test_against = wrapper->Auto.test_against;

        move_required_traits(&follower->Wrapper.Auto.required_traits, type->Wrapper.Auto.required_traits);
        move_required_traits(&follower->Wrapper.Auto.non_matching_required_traits,
                             type->Wrapper.Auto.non_matching_required_traits);
    }

    return 1;
}

static int clash_autos(Type* type, Type* follower, ClashAccumulator* accumulator) {
#ifdef EBUG
    printf("assigning:\t \33[3%dm%-24.*s \33[3%dm%.*s\33[0m\n",
           (int) ((size_t) type->trace.source.data / 16) % 6 + 1,
           fmtof(type->trace.source),
           (int) ((size_t) follower->trace.source.data / 16) % 6 + 1,
           fmtof(follower->trace.source));
#endif

    if(type == follower) {
        return 1;
    }

    if((type->id == WrapperAuto && type->Wrapper.Auto.test_against)
       || (follower->id == WrapperAuto && follower->Wrapper.Auto.test_against)) {
        Type* const type_test = type->id == WrapperAuto && type->Wrapper.Auto.test_against
                                    ? type->Wrapper.Auto.test_against
                                    : type;
        Type* const follower_test = follower->id == WrapperAuto && follower->Wrapper.Auto.test_against
                                        ? follower->Wrapper.Auto.test_against
                                        : follower;

        if(type == follower_test || follower == type_test) {
            return 1;
        }

        if(traverse_type(follower_test, NULL, &circular_acceptor, type, TraverseGenerics | TraverseIntermediate, 1)) {
            accumulator->circular_type = follower_test;
            return TestCircular;
        }

        const unsigned flags_save = accumulator->flags;
        const int test_result = traverse_type(type_test, follower_test, &clash_acceptor, accumulator,
                                              accumulator->flags |= ClashPassive, 0);
        accumulator->flags = flags_save;
        // const int test_result = clash_types(type_test, follower_test, accumulator->trace, accumulator->messages,
        //                                     ClashPassive | accumulator->flags);
        if(test_result) {
            return test_result + 1;
        }
    }

    // TODO: try making this one way (like above)
    bool first = false;
    if(((first = traverse_type(type, NULL, &circular_acceptor, follower, TraverseGenerics | TraverseIntermediate, 0)))
       || traverse_type(follower, NULL, &circular_acceptor, type, TraverseGenerics | TraverseIntermediate, 1)) {
        accumulator->circular_type = first ? type : follower;
        return TestCircular;
    }

    return assign_auto_ref(type, follower, accumulator, accumulator->flags & ClashPassive);
}

static int clash_acceptor(Type* type, Type* follower, void* void_accumulator) {
#ifdef EBUG
    printf("\33[90mclash:\t\t %-24.*s %.*s\33[0m\n",
           fmtof(type->trace.source), fmtof(follower->trace.source));
#endif

    ClashAccumulator* const accumulator = void_accumulator;

    if(type->id == WrapperAuto || follower->id == WrapperAuto) {
        return clash_autos(type, follower, accumulator);
    }

    if(type->id == follower->id)
        switch(type->id) {
            case NodeExternal:
                if(streq(type->External.data, follower->External.data)) {
                    return 1;
                }
                break;

            default:
                return 0;
        }

    if(type->flags & follower->flags & fNumeric) return 1;
    return TestMismatch;
}

int clash_types(Type* a, Type* b, const Trace trace, Vec(Message)* messages, const unsigned flags) {
    ClashAccumulator accumulator = { trace, messages, {}, flags };
    const int result = traverse_type(a, b, &clash_acceptor, &accumulator, 0, 0);

    if(result && !(flags & ClashPassive)) {
        String message = strf(0, iftty("type mismatch between "HERR, "type mismatch between ")).as_owned;
        stringify_type(a, &message, 0, 0);

        strf(&message, iftty(H" and "HERR, " and "));
        stringify_type(b, &message, 0, 0);

        strf(&message, iftty(H, ""));
        push(messages, MERROR(trace, as_str(message)));

        if(result + 1 == TestCircular) {
            String info = strf(0, iftty("types are referencing each-other circularly, see "HINF,
                                   "types are referencing each-other circularly, see")).as_owned;
            stringify_type(accumulator.circular_type, &info, 0, 0);

            push(messages, MINFO(accumulator.circular_type->trace, strf(&info, iftty(H, ""))));
        }
    }

    return result;
}
