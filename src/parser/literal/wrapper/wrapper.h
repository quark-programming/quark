#ifndef WRAPPER_H
#define WRAPPER_H

#include <vector.h>

#include "../../nodes.h"

enum {
    ActionNone,
    ActionApplyGenerics,
    ActionApplyCollection,
};

typedef Vector(struct Action) ActionVector;

typedef struct Action {
    unsigned type;

    union {
        TypeVector generics;
        ActionVector collection;
    };

    Declaration* target;
} Action;

// TODO: Maybe make `NodeWrapper` a flag rather than this confusing type
enum {
    WrapperVariable = NodeWrapper + 1,
    WrapperAuto,
    WrapperSurround,
};

typedef struct Wrapper {
    NODE_FIELDS;

    Action action;

    union {
        struct {
            Declaration* declaration;
            Node* bound_self_argument;
        } Variable;

        struct {
            Type* ref;
            Type* parent_base_generic;
            struct GenericType* replacement_generic;
        } Auto;

        struct {
            String prefix;
            String postfix;
        } Surround;
    };
} Wrapper;

#endif