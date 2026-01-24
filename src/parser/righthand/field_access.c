#include "field_access.h"

#include "function_call.h"
#include "righthand.h"
#include "../lefthand/reference.h"
#include "../type/types.h"
#include "../statement/scope.h"
#include "declaration/declaration.h"
#include "parser/literal/wrapper.h"
#include "../literal/array.h"
#include "declaration/variable.h"

Node* parse_field_access(Node* lefthand, Parser* parser) {
    const String operator_token = next(parser->tokenizer).trace.source;

    if(*operator_token.data == '.' && try(parser->tokenizer, '(', NULL)) {
        Type* cast = (void*) expression(parser);

        if(!(cast->flags & fType)) {
            push(parser->tokenizer->messages, REPORT_ERR(cast->trace, String("cast is not a type")));
            cast = cast->type;
        }

        return new_node((Node) {
            .Cast = {
                .id = NodeCast,
                .type = cast,
                .trace = stretch(lefthand->trace, expect(parser->tokenizer, ')').trace),
                .value = lefthand,
            },
        });
    }

    Type* type = lefthand->type;
    if(*operator_token.data == '-') {
        type = (void*) dereference((void*) type, lefthand->trace, parser->tokenizer->messages);
    }

    const OpenedType opened = open_type(type, 0);
    StructType* const struct_type = (void*) opened.type;

    const Token field_token = expect(parser->tokenizer, TokenIdentifier);

    if(struct_type->id != NodeStructType) {
        push(parser->tokenizer->messages, REPORT_ERR(lefthand->trace,
                 strf(0, "'\33[35m%.*s\33[0m' is not a structure", FMT(lefthand->trace.source))));
        close_type(opened.actions, 0);
        return lefthand;
    }

    ssize_t found_index = -1;
    for(ssize_t i = 0; i < struct_type->fields.size; i++) {
        if(streq(field_token.trace.source, struct_type->fields.data[i].identifier)) {
            found_index = i;
        }
    }

    if(found_index < 0) {
        Wrapper* child = find_in_scope(*struct_type->static_body, field_token.trace);

        if(child) {
            lefthand->type = make_type_standalone(lefthand->type);
            child->Variable.bound_self_argument = lefthand;
            child->type = make_type_standalone(child->type);

            if(global_actions.size) {
                child->action = child->type->Wrapper.action;
            }

            close_type(opened.actions, 0);
            return (void*) child;
        }

        push(parser->tokenizer->messages,
             REPORT_ERR(field_token.trace, strf(0, "no field named '\33[35m%.*s\33[0m' on struct '\33[35m%.*s\33[0m'",
                 FMT(field_token.trace.source), FMT(lefthand->trace.source))));
        push(parser->tokenizer->messages, see_declaration((void*) struct_type, lefthand->trace));
        return lefthand;
    }

    Type* field_type = make_type_standalone(struct_type->fields.data[found_index].type);
    close_type(opened.actions, 0);

    return new_node((Node) {
        .BinaryOperation = {
            .id = NodeBinaryOperation,
            .flags = fMutable | (lefthand->flags & fConstExpr),
            .trace = stretch(lefthand->trace, field_token.trace),
            .type = field_type,
            .left = lefthand,
            .operator = operator_token,
            .right = new_node((Node) {
                .External = { NodeExternal, .data = field_token.trace.source },
            }),
        }
    });
}

Node* parse_indexing(Node* lefthand, Parser* parser) {
    const Trace trace_start = next(parser->tokenizer).trace;
    Node* const index = expression(parser);
    const Trace trace = stretch(trace_start, expect(parser->tokenizer, ']').trace);

    const OpenedType opened_index = open_type(index->type, 0);
    if(opened_index.type->id == NodeStructType
       && streq(opened_index.type->StructType.parent->identifier.base, String("Range"))) {
        close_type(opened_index.actions, 0);

        Declaration* const slice_declaration = fetch_slice_declaration(parser);

        StructLiteral* const struct_literal = (void*) new_node((Node) {
            .StructLiteral = {
                .id = NodeStructLiteral,
                .trace = trace,
                .type = (void*) variable_of(slice_declaration, trace, 0),
            },
        });

        TypeVector generics = { 0 };
        push(&generics, (void*) dereference((void*) lefthand->type, trace, parser->tokenizer->messages));
        struct_literal->type->Wrapper.action = (Action) { ActionApplyGenerics, generics, slice_declaration };

        Scope* const collector = new_scope(NULL);
        Node* temp_index = create_temp_variable(index, parser, &collector->children);

        Node* const size_node = new_node((Node) {
            .BinaryOperation = {
                .id = NodeBinaryOperation,
                .left = new_node((Node) {
                    .BinaryOperation = {
                        .id = NodeBinaryOperation,
                        .left = temp_index,
                        .operator = String("."),
                        .right = new_node((Node) { .External = { NodeExternal, .data = String("end") } }),
                    },
                }),
                .operator = String("-"),
                .right = new_node((Node) {
                    .BinaryOperation = {
                        .id = NodeBinaryOperation,
                        .left = temp_index,
                        .operator = String("."),
                        .right = new_node((Node) { .External = { NodeExternal, .data = String("start") } }),
                    },
                }),
            },
        });

        Node* const data_node = new_node((Node) {
            .BinaryOperation = {
                .id = NodeBinaryOperation,
                .left = new_node((Node) {
                    .Wrapper = { WrapperSurround, .Surround = { lefthand, String("("), String(")") } }
                }),
                .operator = String("+"),
                .right = new_node((Node) {
                    .BinaryOperation = {
                        .id = NodeBinaryOperation,
                        .left = temp_index,
                        .operator = String("."),
                        .right = new_node((Node) { .External = { NodeExternal, .data = String("start") } }),
                    }
                }),
            },
        });

        push(&struct_literal->field_names, String("size"));
        push(&struct_literal->field_values, size_node);
        push(&struct_literal->field_names, String("data"));
        push(&struct_literal->field_values, data_node);

        collector->result_value = (void*) struct_literal;
        collector->type = struct_literal->type;
        collector->trace = trace;
        return (void*) collector;
    }
    close_type(opened_index.actions, 0);

    Node* const override = operator_override(lefthand->type, lefthand, index, String("index"), index->trace, parser);
    if(override) return override;

    Node* const offset = new_node((Node) {
        .Wrapper = {
            .id = WrapperSurround,
            .type = lefthand->type,
            .Surround = {
                .child = new_node((Node) {
                    .BinaryOperation = {
                        .id = NodeBinaryOperation,
                        .type = lefthand->type,
                        .left = lefthand,
                        .operator = String("+"),
                        .right = index,
                    }
                }),
                .prefix = String("("),
                .postfix = String(")"),
            },
        },
    });

    return dereference(offset, trace, parser->tokenizer->messages);
}
