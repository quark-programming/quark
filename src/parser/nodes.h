#ifndef NODES_H
#define NODES_H

#include <stdint.h>

#include "../tokenizer/tokenizer.h"

typedef struct Compiler Compiler;
typedef union Node Node;
typedef union Type Type;
typedef union Declaration Declaration;

typedef Vector(Node*) NodeVector;
typedef Vector(Type*) TypeVector;
typedef Vector(Declaration*) DeclarationVector;

#define NODE_FIELDS \
    uint32_t id; \
    uint32_t flags; \
    Trace trace; \
    Type* type

enum {
    NodeNone,

    NodeWrapper = 1 << 2,
    NodeNumericLiteral,
    NodeMissing,
    NodeExternal,

    NodeVariableDeclaration,
    NodeBinaryOperation,

    NodeScope,
    NodeStatementWrapper,
};

#include "type/types.h"
#include "type/generics.h"

#include "literal/wrapper/wrapper.h"
#include "literal/numeric_literal.h"
#include "literal/missing.h"
#include "literal/external.h"

#include "righthand/declaration/declarations.h"
#include "righthand/declaration/identifier.h"
#include "righthand/declaration/variable_declaration.h"
#include "righthand/binary_operation.h"

#include "statement/scope.h"
#include "statement/statement_wrapper.h"

#endif //NODES_H