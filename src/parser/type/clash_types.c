#include "clash_types.h"
#include "types.h"
#include "stringify_type.h"
#include "traverse_type.h"
#include "tty.h"
#include "parser/statement/scope.h"

static int circular_acceptor(Type* const type, Type* follower, void* const compare) {
    (void) follower;
    return 2 * (type == compare);
}

static int test_required_traits(Wrapper* left, StructType* right) {
    if(!left->Auto.required_traits || right->id == WrapperAuto) return 0;
    if(right->id != NodeStructType) return 1;

    for(u32 i = 0; i < len(left->Auto.required_traits); i++) {
        switch(left->Auto.required_traits[i]->id) {
            case NodeFunctionDeclaration: {
                Scope* const trait_scope = get(right->traits, left->Auto.required_traits[i]
                                               ->identifier.parent_scope->declaration->identifier.base);
                if(!trait_scope) return 1;
                if(trait_scope->declaration) break;

                Declaration* const trait_function =
                        find_in_scope_unwrapped(*trait_scope, left->Auto.required_traits[i]->identifier.base);
                if(!trait_function) return 1;

                break;
            }

            case NodeStructDeclaration: {
                Scope* const trait_scope = get(right->traits, left->Auto.required_traits[i]->identifier.base);
                if(!trait_scope || !trait_scope->declaration) return 1;
                break;
            }

            default: return 1;
        }
    }

    return 0;
}

static int assign_auto_ref(Type* type, Type* follower, const bool passive) {
    u8 generics_offset = 1;
    Wrapper* wrapper = &type->Wrapper;

    if(type->id != WrapperAuto
       || (follower->id == WrapperAuto && follower->Wrapper.Auto.priority > wrapper->Auto.priority)) {
        wrapper = &follower->Wrapper;
        follower = type;
        generics_offset = 0;
    }

    if(follower->id != WrapperAuto && wrapper->flags & fNumeric && !(follower->flags & fNumeric)) return TestMismatch;
    if(test_required_traits(wrapper, (void*) follower)) return TestMismatch;
    if(passive) return 1;

    wrapper->Auto.ref = make_type_standalone(follower, generics_offset);

    if(follower->id == WrapperAuto) {
        if((wrapper->flags | follower->flags) & fNumeric) {
            wrapper->flags |= fNumeric;
            follower->flags |= fNumeric;
        }
        if(wrapper->Auto.test_against) follower->Wrapper.Auto.test_against = wrapper->Auto.test_against;
        if(wrapper->Auto.required_traits) {
            for(u32 i = 0, j; i < len(wrapper->Auto.required_traits); i++) {
                for(j = 0; wrapper->Auto.required_traits[i] != follower->Wrapper.Auto.required_traits[j]
                           && j < len(follower->Wrapper.Auto.required_traits); j++)
                    ;
                if(j == len(follower->Wrapper.Auto.required_traits)) {
                    push(&follower->Wrapper.Auto.required_traits, wrapper->Auto.required_traits[i]);
                }
            }
        }
    }

    return 1;
}

static int clash_acceptor(Type* type, Type* follower, void* void_accumulator);

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

        if(type == follower_test || follower == type_test
           || traverse_type(follower_test, NULL, &circular_acceptor, type,
                            TraverseGenerics | TraverseIntermediate, 1)) {
            return 1;
        }

        const int test_result = clash_types(type_test, follower_test, accumulator->trace, accumulator->messages,
                                            ClashPassive | accumulator->flags);
        if(test_result) {
            return test_result + 1;
        }
    }

    // TODO: try making this one way (like above)
    if(traverse_type(type, NULL, &circular_acceptor, follower, TraverseGenerics | TraverseIntermediate, 0)
       || traverse_type(follower, NULL, &circular_acceptor, type, TraverseGenerics | TraverseIntermediate, 1)) {
        return TestCircular;
    }

    return assign_auto_ref(type, follower, accumulator->flags & ClashPassive);
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
    ClashAccumulator accumulator = { trace, messages, flags };
    const int result = traverse_type(a, b, &clash_acceptor, &accumulator, 0, 0);

    if(result && !(flags & ClashPassive)) {
        String message = strf(0, iftty("type mismatch between "HERR, "type mismatch between ")).as_owned;
        stringify_type(a, &message, 0, 0);

        strf(&message, iftty(H" and "HERR, " and "));
        stringify_type(b, &message, 0, 0);

        strf(&message, iftty(H, ""));
        push(messages, MERROR(trace, as_str(message)));

        if(result + 1 == TestCircular) {
            push(messages, MINFO({ 0 }, str("types are referencing each-other circularly")));
        }
    }

    return result;
}
