#include "assignment.h"

void check_assignable(Node* node, Vec(Message)* messages) {
    if(node->flags & fMutable && !(node->type->flags & fConst)) return;
    push(messages, MERROR(node->trace,
        strf(0, "'\33[35m%.*s\33[0m' is not assignable", fmtof(node->trace.source))));
}

Node* parse_postfix_assignment(Node* lefthand, Parser* parser) {
    check_assignable(lefthand, parser->tokenizer->messages);
    const Trace operator_trace = next(parser->tokenizer).trace;

    return new_node((Node) {
        .Wrapper = {
            .id = WrapperSurround,
            .trace = stretch(lefthand->trace, operator_trace),
            .type = lefthand->type,
            .Surround = { lefthand, {}, operator_trace.source },
        },
    });
}