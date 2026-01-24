#include "variable.h"

#include "../righthand.h"
#include "../../lefthand/lefthand.h"
#include "../../literal/wrapper.h"
#include "../../statement/statement.h"
#include "../../type/clash_types.h"

Node* parse_variable_declaration(Type* type, IdentifierInfo info, Parser* parser) {
    VariableDeclaration* declaration = (void*) new_node((Node) {
        .VariableDeclaration = {
            .id = NodeVariableDeclaration,
            .trace = stretch(type->trace, info.trace),
            .type = (void*) type,
            .identifier = info.identifier,
        }
    });
    declaration->identifier.parent_declaration = (void*) declaration;

    put(&info.declaration_scope->variables, info.identifier.base, (void*) declaration);

    if(type->flags & fConst && try(parser->tokenizer, '=', 0)) {
        declaration->const_value = righthand_expression(lefthand_expression(parser), parser, 14);
        clash_types(declaration->type, declaration->const_value->type, declaration->trace,
                    parser->tokenizer->messages, 0);
    }

    FunctionDeclaration* const parent = (void*) last(parser->stack)->parent;
    if(parent->id == NodeFunctionDeclaration || parent->id == NodeEntryFunctionDeclaration) {
        push(&parent->variable_declarations, declaration);
        declaration->compilation_state = CompilationHoisted;
    }

    return (void*) variable_of((void*) declaration, declaration->trace, fIgnoreStatement);
}

Node* create_temp_variable(Node* const value, Parser* const parser, NodeVector* collector) {
    static unsigned id = 0;

    Declaration* declaration = (void*) new_node((Node) {
        .VariableDeclaration = {
            .id = NodeVariableDeclaration,
            .trace = value->trace,
            .flags = value->flags,
            .type = value->type,
            .identifier = {
                .base = strf(0, "__qv%u", id++),
                .parent_scope = (void*) last(parser->stack),
            },
        },
    });
    declaration->VariableDeclaration.identifier.parent_declaration = declaration;
    push(collector, (void*) declaration);

    Node* const variable = new_node((Node) {
        .External = {
            .id = NodeExternal,
            .trace = value->trace,
            .flags = value->flags,
            .type = value->type,
            .data = declaration->identifier.base,
        },
    });

    Node* const assignment_statement = new_node((Node) {
        .StatementWrapper = {
            .id = NodeStatementWrapper,
            .expression = new_node((Node) {
                .BinaryOperation = {
                    .id = NodeBinaryOperation,
                    .left = variable,
                    .operator = String("="),
                    .right = value,
                },
            }),
        },
    });
    push(collector, assignment_statement);

    return variable;
}
