#ifndef NODES_H
#define NODES_H

#include <wrap.h>
#include "../../tokenizer/tokenizer.h"
#include "fields.h"

typedef enum : u32 {
    NodeNone,

    NodeWrapper = 1 << 2,
    WrapperVariable = NodeWrapper + 1,
    WrapperAuto,
    WrapperSurround,

    NodeFunctionType = 1 << 3,
    NodePointerType,
    NodeGenericReference,
    NodeStructType,

    NodeCast,

    NodeNumericLiteral,
    NodeStructLiteral,
    NodeMissing,
    NodeExternal,

    NodeBinaryOperation,
    NodeFunctionCall,
    NodeVariableDeclaration,
    NodeFunctionDeclaration,
    NodeEntryFunctionDeclaration,
    NodeStructDeclaration,
    NodeDeclarationLink,

    NodeScope,
    NodeModule,
    NodeStatementWrapper,
    NodeReturnStatement,
    NodeControlStatement,
} NodeID;

enum {
    CompilationUnused,
    CompilationSkip,
    CompilationIntermediate,
    CompilationLocal,
};

enum {
    fType = 1 << 0,
    fConst = 1 << 1,
    fConstExpr = 1 << 2,
    fMutable = 1 << 3,
    fIgnoreStatement = 1 << 4,
    fStatementTerminated = 1 << 5,
    fNumeric = 1 << 6,
    fPrivate = 1 << 7,
};

#include "righthand/declaration/identifier.h"
#include "statement/scope.h"

#include "type/generics.h"
#include "type/function_type.h"
#include "type/pointer_type.h"
#include "type/struct_type.h"

#include "lefthand/cast.h"

#include "literal/wrapper.h"
#include "literal/numeric_literal.h"
#include "literal/struct_literal.h"
#include "literal/missing.h"
#include "literal/external.h"

#include "righthand/binary_operation.h"
#include "righthand/function_call.h"
#include "righthand/declaration/variable_declaration.h"
#include "righthand/declaration/function_declaration.h"
#include "righthand/declaration/struct_declaration.h"
#include "righthand/declaration/link.h"

#include "statement/statement_wrapper.h"
#include "statement/return_statement.h"
#include "statement/control_statement.h"
#include "statement/module.h"

union Type {
    struct { TYPE_FIELDS; };

    FunctionType FunctionType;
    PointerType PointerType;
    GenericReference GenericReference;
    StructType StructType;

    Wrapper Wrapper;
    External External;
    Missing Missing;
};

union Declaration {
    struct { DECLARATION_FIELDS; };

    FunctionDeclaration FunctionDeclaration;
    VariableDeclaration VariableDeclaration;
    StructDeclaration StructDeclaration;
    DeclarationLink DeclarationLink;
};

union Node {
    struct { NODE_FIELDS; };

    FunctionType FunctionType;
    PointerType PointerType;
    GenericReference GenericReference;
    StructType StructType;

    Wrapper Wrapper;
    Cast Cast;
    NumericLiteral NumericLiteral;
    StructLiteral StructLiteral;
    Missing Missing;
    External External;

    BinaryOperation BinaryOperation;
    FunctionCall FunctionCall;
    VariableDeclaration VariableDeclaration;
    FunctionDeclaration FunctionDeclaration;
    StructDeclaration StructDeclaration;

    StatementWrapper StatementWrapper;
    ReturnStatement ReturnStatement;
    ControlStatement ControlStatement;
    Scope Scope;
    Module Module;

    Type Type;
    Declaration Declaration;
};

extern Vec(Vec(Node)) global_node_arena;
extern Vec(Node*) global_unused_nodes;

void init_node_arena(size_t initial_size);

Node* new_node(Node node);

void unbox(Node* box);

#endif //NODES_H