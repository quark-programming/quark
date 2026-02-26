#include "reference.h"
#include "../type/types.h"

Node* reference(Node* node, const Trace trace) {
    if(node->flags & fType) {
        return (void*) new_type((Type) {
            .PointerType = {
                .id = NodePointerType,
                .flags = fNumeric,
                .trace = trace,
                .base = (void*) node,
            }
        });
    }

    return new_node((Node) {
        .Wrapper = {
            .id = WrapperSurround,
            .trace = trace,
            .type = (void*) reference((void*) node->type, trace),
            .Surround = { node, str("(&"), str(")") },
        }
    });
}

Node* dereference(Node* node, Trace trace, Vec(Message)* messages) {
    if(node->flags & fType) {
        const OpenedType open = open_type((void*) node, 0, 0);

        if(open.type->id != NodePointerType) {
            push(messages, MERROR(trace, strf(0, "Cannot de-refence a non-pointer value")));
            close_type(open.actions, 0, 0);
            return node;
        }

        Type* const child = make_type_standalone(open.type->PointerType.base);
        close_type(open.actions, 0, 0);
        return (void*) child;
    }

    return new_node((Node) {
        .Wrapper = {
            .id = WrapperSurround,
            .flags = fMutable,
            .trace = trace,
            .type = (void*) dereference((void*) node->type, trace, messages),
            .Surround = { node, str("(*"), str(")") },
        }
    });
}

