#include "clash_types.h"
#include "types.h"
#include "stringify_type.h"
#include "traverse_type.h"
#include "tty.h"

static int circular_acceptor(Type* const type, Type* follower, void* const compare) {
    (void) follower;
    return 2 * (type == compare);
}

static void assign_auto_ref(Wrapper* wrapper, Type* follower) {
    wrapper->Auto.ref = make_type_standalone(follower);

    if(follower->id == WrapperAuto) {
        if((wrapper->flags | follower->flags) & fNumeric) {
            wrapper->flags |= fNumeric;
            follower->flags |= fNumeric;
        }
        if(wrapper->Auto.test_against) follower->Wrapper.Auto.test_against = wrapper->Auto.test_against;
    }
}

static int clash_acceptor(Type* type, Type* follower, void* void_accumulator);

static int clash_autos(Wrapper* wrapper, Type* follower, ClashAccumulator* accumulator) {
#ifdef EBUG
    printf("assigning:\t \33[3%dm%-24.*s \33[3%dm%.*s\33[0m\n",
           (int) ((size_t) wrapper->trace.source.data / 16) % 6 + 1,
           fmtof(wrapper->trace.source),
           (int) ((size_t) follower->trace.source.data / 16) % 6 + 1,
           fmtof(follower->trace.source));
#endif

    if((void*) wrapper == follower) {
        return 1;
    }

    if(wrapper->Auto.test_against) {
        const OpenedType test_against = open_type(wrapper->Auto.test_against, 0);
        OpenedType follower_test_against = { 0 };

        if(test_against.type == follower) {
            close_type(test_against.actions, 0);
            close_type(follower_test_against.actions, 0);
            return 1;
        }

        if(follower->id == WrapperAuto && follower->Wrapper.Auto.test_against) {
            follower_test_against = open_type(follower->Wrapper.Auto.test_against, 0);
            follower = follower_test_against.type;

            if(test_against.type == follower) {
                close_type(test_against.actions, 0);
                close_type(follower_test_against.actions, 0);
                return 1;
            }
        }

        const int clash_error = clash_types(test_against.type, follower, accumulator->trace, accumulator->messages,
                                            ClashPassive | accumulator->flags);
        if(clash_error) {
            close_type(test_against.actions, 0);
            close_type(follower_test_against.actions, 0);
            return clash_error + 1;
        }

        if(!(accumulator->flags & ClashPassive)) assign_auto_ref(wrapper, follower);
        close_type(test_against.actions, 0);
        close_type(follower_test_against.actions, 0);
        return 1;
    }

    if(wrapper->flags & fNumeric && !(follower->flags & fNumeric) && follower->id != WrapperAuto) {
        return TestMismatch;
    }

    if(traverse_type((void*) wrapper, NULL, &circular_acceptor, follower, TraverseGenerics & TraverseIntermediate) ||
       traverse_type((void*) follower, NULL, &circular_acceptor, wrapper, TraverseGenerics & TraverseIntermediate)) {
        return TestCircular;
    }

    if(!(accumulator->flags & ClashPassive)) assign_auto_ref(wrapper, follower);

    return 1;
}

static int clash_acceptor(Type* type, Type* follower, void* void_accumulator) {
#ifdef EBUG
    printf("\33[90mclash:\t\t %-24.*s %.*s\33[0m\n",
           fmtof(type->trace.source), fmtof(follower->trace.source));
#endif

    ClashAccumulator* const accumulator = void_accumulator;

    if(follower->id == WrapperAuto && !(type->id == WrapperAuto && type->Wrapper.Auto.test_against)) {
        return clash_autos((void*) follower, type, accumulator);
    }

    if(type->id == WrapperAuto) {
        return clash_autos((void*) type, follower, accumulator);
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
    const int result = traverse_type(a, b, &clash_acceptor, &accumulator, 0);

    if(result && !(flags & ClashPassive)) {
        String message = strf(0, iftty("type mismatch between '\33[35m", "type mismatch between '")).as_owned;
        stringify_type(a, &message, 0);

        strf(&message, iftty("\33[0m' and '\33[35m", "' and '"));
        stringify_type(b, &message, 0);

        strf(&message, result + 1 == TestCircular
             ? iftty("\33[0m' (types are circularly referencing each other)",
                 "' (types are circularly referencing each other)")
             : iftty("\33[0m'", "'"));

        push(messages, MERROR(trace, as_str(message)));
    }

    return result;
}
