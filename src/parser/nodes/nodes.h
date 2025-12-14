#ifndef NODES_H
#define NODES_H

#include <stdint.h>

#include "../../tokenizer/tokenizer.h"
#include "fields.h"

enum {
    NodeNone,

    NodeWrapper = 1 << 2,
    NodeFunctionType,
    NodePointerType,
    NodeGenericReference,
    NodeStructType,

    NodeNumericLiteral,
    NodeStructLiteral,
    NodeMissing,
    NodeExternal,

    NodeBinaryOperation,
    NodeFunctionCall,
    NodeVariableDeclaration,
    NodeFunctionDeclaration,

    NodeScope,
    NodeStatementWrapper,
    NodeReturnStatement,
    NodeControlStatement,
};

enum {
    fType = 1 << 0,
    fConst = 1 << 1,
    fConstExpr = 1 << 2,
    fMutable = 1 << 3,
    fIgnoreStatement = 1 << 4,
    fStatementTerminated = 1 << 5,
    fExternal = 1 << 6,
    fNumeric = 1 << 7,
};

#include "righthand/declaration/identifier.h"
#include "statement/scope.h"

#include "type/generics.h"
#include "type/function_type.h"
#include "type/pointer_type.h"
#include "type/struct_type.h"

#include "literal/wrapper.h"
#include "literal/numeric_literal.h"
#include "literal/struct_literal.h"
#include "literal/missing.h"
#include "literal/external.h"

#include "righthand/binary_operation.h"
#include "righthand/function_call.h"
#include "righthand/declaration/variable_declaration.h"
#include "righthand/declaration/function_declaration.h"

#include "statement/statement_wrapper.h"
#include "statement/return_statement.h"
#include "statement/control_statement.h"

union Type {
    struct { TYPE_FIELDS; };

    FunctionType FunctionType;
    PointerType PointerType;
    GenericReference GenericReference;
    StructType StructType;

    Wrapper Wrapper;
};

union Declaration {
    struct { DECLARATION_FIELDS; };

    FunctionDeclaration FunctionDeclaration;
    VariableDeclaration VariableDeclaration;
};

union Node {
    struct { NODE_FIELDS; };

    FunctionType FunctionType;
    PointerType PointerType;
    GenericReference GenericReference;
    StructType StructType;

    Wrapper Wrapper;
    NumericLiteral NumericLiteral;
    StructLiteral StructLiteral;
    Missing Missing;
    External External;

    BinaryOperation BinaryOperation;
    FunctionCall FunctionCall;
    VariableDeclaration VariableDeclaration;
    FunctionDeclaration FunctionDeclaration;

    StatementWrapper StatementWrapper;
    ReturnStatement ReturnStatement;
    ControlStatement ControlStatement;
    Scope Scope;

    Type Type;
    Declaration Declaration;
};

typedef Vector(Node) AbsoluteNodeVector;
typedef Vector(AbsoluteNodeVector) NodeArena;

extern NodeArena global_node_arena;
extern NodeVector global_unused_nodes;

void init_node_arena(size_t initial_size);

Node* new_node(Node node);

void unbox(Node* box);

#endif //NODES_H