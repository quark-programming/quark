#ifndef CLASH_TYPES_H
#define CLASH_TYPES_H

#include "../nodes/nodes.h"

enum {
    TestMismatch = 2,
    TestCircular,

    ClashPassive = 1 << 0,
};

typedef struct ClashAccumulator {
    Trace trace;
    Vec(Message)* messages;
    unsigned flags;
    u8 assign_auto_generic_offset;
} ClashAccumulator;

int clash_types(Type* a, Type* b, Trace trace, Vec(Message)* messages, unsigned flags);

#endif