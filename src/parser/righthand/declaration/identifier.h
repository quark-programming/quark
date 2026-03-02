#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include "../../nodes/nodes.h"
#include "../../parser.h"
#include "../../type/generics.h"

enum {
    IdentifierDeclaration = 1 << 0,
};

typedef struct IdentifierInfo {
    Identifier identifier;
    Wrapper* value;
    Scope* declaration_scope;
    Scope* trait_scope;
    Trace trace;
    GenericsCollection generics_collection;
    Vec(struct Action) declaration_actions;
} IdentifierInfo;

IdentifierInfo new_identifier(Token base_identifier, Parser* parser, unsigned flags);

#endif