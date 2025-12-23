#ifndef NODE_WRAPPER_H
#define NODE_WRAPPER_H

#include <vector.h>
#include <vector-string.h>

#include "../fields.h"

enum {
    ActionNone,
    ActionApplyGenerics,
    ActionApplyCollection,
};

typedef Vector(struct Action, ActionVector) ActionVector;

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
            Node* child;
        };

        struct {
            Declaration* declaration;
            Node* bound_self_argument;
            bool is_self_literal : 1;
        } Variable;

        struct {
            Type* ref;
            Type* parent_base_generic;
            struct GenericReference* replacement_generic;
        } Auto;

        struct {
            Node* child;
            String prefix;
            String postfix;
            bool no_parenthesis_wrap : 1;
        } Surround;
    };
} Wrapper;

#endif
