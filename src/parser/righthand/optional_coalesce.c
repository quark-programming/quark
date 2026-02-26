#include <clargs.h>

#include "optional_coalesce.h"

#include "righthand.h"
#include "../statement/scope.h"
#include "declaration/variable.h"

#include "../statement/statement.h"
#include "../type/clash_types.h"
#include "../type/types.h"

Node* parse_optional_coalescing(Node* lefthand, Parser* parser) {
    static Declaration* optional_declaration = NULL;

    if(!optional_declaration) {
        optional_declaration = find_on_stack_unwrapped(parser->stack, str("Option"));

        if(!optional_declaration) {
            panicf("[fatal] Unable to find definition for Option<T>\n");
        }
    }

    Type* const optional_type = new_type((Type) {
        .Wrapper = {
            .id = WrapperAuto,
            .action = { ActionApplyGenerics, {}, optional_declaration },
            .Auto.ref = (void*) optional_declaration->const_value,
        },
    });

    next(parser->tokenizer);

    if(lefthand->flags & fType) {
        push(&optional_type->Wrapper.action.generics, (void*) lefthand);
        return (void*) optional_type;
    }

    const RighthandOperator operator = global_righthand_operator_table[parser->tokenizer->current.type];
    if(operator.precedence > 1) return NULL;

    const OpenedType lefthand_optional = open_type(lefthand->type, 0, 2);

    if(lefthand_optional.type->id != NodeStructType ||
       !streq(lefthand_optional.type->StructType.module->declaration->identifier.base, str("Option"))) {
        close_type(lefthand_optional.actions, 0, 2);
        return NULL;
    }

    Type* const lefthand_value_type =
            last(lefthand_optional.type->StructType.module->declaration->generics.type_arguments_stacks[2])[0];
    close_type(lefthand_optional.actions, 0, 2);

    Scope* operation_step_collection = (void*) new_node((Node) {
        .Scope = {
            .id = NodeScope,
            .trace = lefthand->trace,
            .wrap_with_brackets = true,
        }
    });

    Node* const lefthand_temp_value = create_temp_variable(lefthand, parser, &operation_step_collection->children);

    Node* const lefthand_optional_value = righthand_expression(
        new_node((Node) {
            .BinaryOperation = {
                .id = NodeBinaryOperation,
                .type = lefthand_value_type,
                .trace = lefthand->trace,
                .left = lefthand_temp_value,
                .operator = str("."),
                .right = new_node((Node) { .External = { NodeExternal, .data = str("value") } }),
            },
        }), parser, 2);

    Node* const some_branch_if_statement = new_node((Node) {
        .ControlStatement = {
            .id = NodeControlStatement,
            .keyword = str("if"),
            .body = new_scope(NULL),
        },
    });

    Node* const if_cond = new_node((Node) {
        .BinaryOperation = {
            .id = NodeBinaryOperation,
            .left = lefthand_temp_value,
            .operator = str("."),
            .right = new_node((Node) { .External = { NodeExternal, .data = str("some") } }),
        }
    });
    push(&some_branch_if_statement->ControlStatement.conditions, if_cond);

    const OpenedType opened_optional_value = open_type(lefthand_optional_value->type, 0, 2);
    const bool no_resulting_value = opened_optional_value.type->id == NodeExternal
                                    && streq(opened_optional_value.type->External.data, str("void"));
    close_type(opened_optional_value.actions, 0, 2);

    if(no_resulting_value) {
        push(&operation_step_collection->children, some_branch_if_statement);
        push(&some_branch_if_statement->ControlStatement.body->children, lefthand_optional_value);
        return (void*) operation_step_collection;
    }

    static Node false_node = { .External = { NodeExternal, .data = str("false") } };
    static Node true_node = { .External = { NodeExternal, .data = str("true") } };

    push(&optional_type->Wrapper.action.generics, lefthand_optional_value->type);

    Node* const operation_result_value = new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = optional_type,
        },
    });
    push(&operation_result_value->StructLiteral.field_names, str("some"));
    push(&operation_result_value->StructLiteral.field_values, &false_node);

    Node* const operation_temp_result = create_temp_variable(operation_result_value, parser,
                                                             &operation_step_collection->children);
    push(&operation_step_collection->children, some_branch_if_statement);

    Node* const some_branch_result_optional = new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = optional_type,
        },
    });
    push(&some_branch_result_optional->StructLiteral.field_names, str("some"));
    push(&some_branch_result_optional->StructLiteral.field_values, &true_node);
    push(&some_branch_result_optional->StructLiteral.field_names, str("value"));
    push(&some_branch_result_optional->StructLiteral.field_values, lefthand_optional_value);

    Node* const some_branch_assignment = new_node((Node) {
        .StatementWrapper = {
            .id = NodeStatementWrapper,
            .expression = new_node((Node) {
                .BinaryOperation = {
                    .id = NodeBinaryOperation,
                    .left = operation_temp_result,
                    .operator = str("="),
                    .right = some_branch_result_optional,
                },
            }),
        },
    });
    push(&some_branch_if_statement->ControlStatement.body->children, some_branch_assignment);

    operation_step_collection->result_value = operation_temp_result;
    operation_step_collection->type = operation_temp_result->type;
    return (void*) operation_step_collection;
}
