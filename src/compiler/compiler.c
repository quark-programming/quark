#include "compiler.h"

#include "literal/literals.h"
#include "literal/wrapper.h"
#include "righthand/righthand.h"
#include "righthand/declaration/function_declaration.h"
#include "righthand/declaration/struct_declaration.h"
#include "righthand/declaration/variable_declaration.h"
#include "statement/scope.h"
#include "statement/statement.h"
#include "types/types.h"

String new_line(Compiler* const compiler) {
    const String indent = compiler->sections[compiler->open_section].indent;
    return strf(0, "%.*s", fmtof(indent)).as_owned;
}

void compile(void* void_node, String* line, Compiler* compiler) {
    static void (*const compiler_function_table[])(void*, String*, Compiler*) = {
        [NodeNumericLiteral] = &comp_NumericLiteral,
        [WrapperAuto] = &comp_Auto,
        [WrapperVariable] = &comp_Variable,
        [WrapperSurround] = &comp_Surround,
        [NodeScope] = &comp_Scope,
        // [NodeMissing] = &comp_Missing,
        [NodeExternal] = &comp_External,
        [NodeGenericReference] = &comp_GenericReference,
        [NodeVariableDeclaration] = &comp_VariableDeclaration,
        [NodeBinaryOperation] = &comp_BinaryOperation,
        [NodeFunctionCall] = &comp_FunctionCall,
        [NodePointerType] = &comp_PointerType,
        [NodeFunctionType] = &comp_FunctionType,
        [NodeFunctionDeclaration] = &comp_FunctionDeclaration,
        [NodeEntryFunctionDeclaration] = &comp_FunctionDeclaration,
        [NodeStatementWrapper] = &comp_StatementWrapper,
        [NodeReturnStatement] = &comp_ReturnStatement,
        [NodeControlStatement] = &comp_ControlStatement,
        [NodeStructType] = &comp_StructType,
        [NodeStructLiteral] = &comp_StructLiteral,
        [NodeCast] = &comp_Cast,
        [NodeStructDeclaration] = &comp_StructDeclaration,
    };

    Node* const node = void_node;
    if(!node->id) return;
    compiler_function_table[node->id](node, line, compiler);
}