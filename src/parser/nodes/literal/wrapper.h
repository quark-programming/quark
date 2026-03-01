#ifndef NODE_WRAPPER_H
#define NODE_WRAPPER_H

#include "../fields.h"

enum {
    ActionNone,
    ActionApplyGenerics,
    ActionApplyCollection,
};

typedef struct Action {
    unsigned type;

    union {
        Vec(Type*) generics;
        Vec(struct Action) collection;
    };

    Declaration* target;
} Action;

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
            bool is_self_literal;
        } Variable;

        struct {
            Type* ref;
            Type* test_against;
            Vec(Declaration*) required_traits;
            i32 priority;
            bool constant;
        } Auto;

        struct {
            Node* child;
            str prefix;
            str postfix;
        } Surround;
    };
} Wrapper;

#endif
