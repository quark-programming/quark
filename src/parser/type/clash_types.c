#include "clash_types.h"
#include "types.h"
#include "stringify_type.h"

static int circular_acceptor(Type* type, Type* follower, void* compare) {
    (void) follower;
    return 2 * (type == compare);
}


static int assign_wrapper(Wrapper* wrapper, Type* follower, ClashAccumulator* accumulator) {
#ifdef EBUG
    printf("assigning:\t \33[3%dm%-24.*s \33[3%dm%.*s\33[0m\n",
           (int) ((size_t) wrapper->trace.source.data / 16) % 6 + 1,
           (int) wrapper->trace.source.size, wrapper->trace.source.data,
           (int) ((size_t) follower->trace.source.data / 16) % 6 + 1,
           (int) follower->trace.source.size, follower->trace.source.data);
#endif

    if(wrapper->flags & fNumeric && !(follower->flags & fNumeric) && follower->id != WrapperAuto) {
        return TestMismatch;
    }

    if((void*) wrapper == follower) {
        return 1;
    }

    if(traverse_type((void*) wrapper, NULL, &circular_acceptor, follower, TraverseGenerics & TraverseIntermediate) ||
       traverse_type((void*) follower, NULL, &circular_acceptor, wrapper, TraverseGenerics & TraverseIntermediate)) {
        return TestCircular;
    }

    if(wrapper->Auto.parent_base_generic) {
        const int result = clash_types(wrapper->Auto.parent_base_generic, follower, accumulator->trace,
                                       accumulator->messages,
                                       ClashPassive | accumulator->flags);
        if(result) return result + 1;
    }

    if(!(accumulator->flags & ClashPassive)) {
        if(follower->id == WrapperAuto && follower->Wrapper.Auto.replacement_generic) {
            if(traverse_type(follower, NULL, (void*) &circular_acceptor, wrapper->Auto.parent_base_generic,
                             TraverseGenerics & TraverseIntermediate)) {
                wrapper->Auto.replacement_generic = follower->Wrapper.Auto.replacement_generic;
                return 1;
            }

            const OpenedType anchor = open_type((void*) follower->Wrapper.Auto.replacement_generic, 0);
            wrapper->Auto.ref = make_type_standalone(anchor.type);
            close_type(anchor.actions, 0);
        } else {
            wrapper->Auto.ref = make_type_standalone(follower);
        }

        if(wrapper->flags & fNumeric && !(follower->flags & fNumeric)) {
            follower->flags = wrapper->flags;
        } else {
            wrapper->flags = follower->flags;
        }

        if(follower->id == WrapperAuto) {
            if(wrapper->Auto.parent_base_generic) {
                follower->Wrapper.Auto.parent_base_generic = wrapper->Auto.parent_base_generic;
            }

            if(wrapper->Auto.replacement_generic) {
                follower->Wrapper.Auto.replacement_generic = wrapper->Auto.replacement_generic;
            }
        }
    }

    return 1;
}

static int clash_acceptor(Type* type, Type* follower, void* void_accumulator) {
#ifdef EBUG
    printf("\33[90mclash:\t\t %-24.*s %.*s\33[0m\n",
           (int) type->trace.source.size, type->trace.source.data,
           (int) follower->trace.source.size, follower->trace.source.data);
#endif

    ClashAccumulator* const accumulator = void_accumulator;

    if(type->id == WrapperAuto && !(type->Wrapper.Auto.replacement_generic && follower->id == WrapperAuto)) {
        return assign_wrapper((void*) type, follower, accumulator);
    }

    if(follower->id == WrapperAuto) {
        return assign_wrapper((void*) follower, type, accumulator);
    }

    if(type->id != follower->id) {
    } else if(type->id == NodeExternal && streq(type->External.data, follower->External.data)) {
        return 1;
    } else {
        return 0;
    }

    if(type->flags & follower->flags & fNumeric) return 1;
    return TestMismatch;
}

int clash_types(Type* a, Type* b, Trace trace, MessageVector* messages, unsigned flags) {
    ClashAccumulator accumulator = { trace, messages, flags };
    const int result = traverse_type(a, b, &clash_acceptor, &accumulator, 0);

    if(result && !(flags & ClashPassive)) {
        String message = strf(0, "type mismatch between '\33[35m");
        stringify_type(a, &message, 0);

        strf(&message, "\33[0m' and '\33[35m");
        stringify_type(b, &message, 0);

        strf(&message, result + 1 == TestCircular
                           ? "\33[0m' (types are circularly referencing each other)"
                           : "\33[0m'");

        push(messages, REPORT_ERR(trace, message));
    }

    return result;
}
