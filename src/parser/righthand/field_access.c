#include "field_access.h"

#include "function_call.h"
#include "righthand.h"
#include "tty.h"
#include "../lefthand/reference.h"
#include "../type/types.h"
#include "../statement/scope.h"
#include "declaration/declaration.h"
#include "parser/literal/wrapper.h"
#include "../literal/array.h"
#include "declaration/variable.h"
#include "parser/literal/number.h"

Node* parse_field_access(Node* lefthand, Parser* parser) {
    const str operator_token = next(parser->tokenizer).trace.source;

    if(*operator_token.data == '.' && try(parser->tokenizer, '(', NULL)) {
        Type* const type = get_type(parser);
        return new_node((Node) {
            .Cast = {
                .id = NodeCast,
                .type = type,
                .trace = stretch(lefthand->trace, expect(parser->tokenizer, ')').trace),
                .value = lefthand,
            },
        });
    }

    Type* type = lefthand->type;
    if(*operator_token.data == '-') {
        type = (void*) dereference((void*) type, lefthand->trace, parser->tokenizer->messages);
    }

    const OpenedType opened = open_type(type, 0, 2);
    StructType* const struct_type = (void*) opened.type;

    const Token field_token = expect(parser->tokenizer, TokenIdentifier);

    if(struct_type->id != NodeStructType) {
        if(opened.type->id == WrapperAuto && opened.type->Wrapper.Auto.required_traits) {
            Vec(Declaration*) required_traits = opened.type->Wrapper.Auto.required_traits;

            for(u32 i = 0; i < len(required_traits); i++) {
                switch(required_traits[i]->id) {
                    case NodeFunctionDeclaration:
                        if(streq(required_traits[i]->identifier.base, field_token.trace.source)) {
                            Node* const trait_access = new_node((Node) {
                                .TraitAccess = {
                                    .id = NodeTraitAccess,
                                    .trace = stretch(lefthand->trace, field_token.trace),
                                    .type = make_type_standalone(required_traits[i]->type, 2),
                                    .generic_type = make_type_standalone(opened.type, 2),
                                    .trait_declaration = required_traits[i]->identifier.parent_scope->declaration,
                                    .field_trace = field_token.trace,
                                    .bound_self_argument = lefthand,
                                },
                            });

                            close_type(opened.actions, 0, 2);
                            return trait_access;
                        }
                        break;

                    case NodeStructDeclaration: {
                        Declaration* const child =
                                find_in_scope_unwrapped(*required_traits[i]->type->StructType.module->scope,
                                                        field_token.trace.source);
                        if(!child) break;

                        Node* const trait_access = new_node((Node) {
                            .TraitAccess = {
                                .id = NodeTraitAccess,
                                .trace = stretch(lefthand->trace, field_token.trace),
                                .type = make_type_standalone(child->type, 2),
                                .generic_type = make_type_standalone(opened.type, 2),
                                .trait_declaration = required_traits[i],
                                .field_trace = field_token.trace,
                                .bound_self_argument = lefthand,
                            },
                        });

                        close_type(opened.actions, 0, 2);
                        return trait_access;
                    }

                    default: ;
                }
            }
        }

        push(parser->tokenizer->messages, MERROR(lefthand->trace,
                 strf(0, iftty(HERR"%.*s"H" is not a structure", "%.*s is not a structure"),
                     fmtof(lefthand->trace.source))));
        close_type(opened.actions, 0, 2);
        return lefthand;
    }

    ssize_t found_index = -1;
    for(ssize_t i = 0; i < len(struct_type->fields); i++) {
        if(streq(field_token.trace.source, struct_type->fields[i].identifier)
           && !(struct_type->fields[i].private && struct_type->module->scope->flags & fPrivate)) {
            found_index = i;
        }
    }

    if(found_index < 0) {
        Wrapper* child = find_in_scope(*struct_type->module->scope, field_token.trace);

        if(child) {
            lefthand->type = make_type_standalone(lefthand->type, 2);
            child->Variable.bound_self_argument = lefthand;

            if(len(global_actions[2])) {
                assign_action((void*) child, lefthand->type->Wrapper.action, true, true);
            }

            close_type(opened.actions, 0, 2);
            return (void*) child;
        }

        push(parser->tokenizer->messages,
             MERROR(field_token.trace, strf(0, iftty("no field named "HERR"%.*s"H" on struct "HERR"%.*s"H,
                     "no field named %.*s on struct %.*s"),
                 fmtof(field_token.trace.source), fmtof(lefthand->trace.source))));
        push(parser->tokenizer->messages, see_declaration((void*) struct_type, lefthand->trace));
        return lefthand;
    }

    Type* field_type = make_type_standalone(struct_type->fields[found_index].type, 2);
    close_type(opened.actions, 0, 2);

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

    const OpenedType opened_index = open_type(index->type, 0, 2);
    if(opened_index.type->id == NodeStructType
       && streq(opened_index.type->StructType.module->declaration->identifier.base, str("Range"))) {
        close_type(opened_index.actions, 0, 2);

        Declaration* const slice_declaration = fetch_slice_declaration(parser);

        StructLiteral* const struct_literal = (void*) new_node((Node) {
            .StructLiteral = {
                .id = NodeStructLiteral,
                .trace = trace,
                .type = (void*) variable_of(slice_declaration, trace, 0),
            },
        });

        Vec(Type*) generics = { 0 };
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
                        .operator = str("."),
                        .right = new_node((Node) { .External = { NodeExternal, .data = str("end") } }),
                    },
                }),
                .operator = str("-"),
                .right = new_node((Node) {
                    .BinaryOperation = {
                        .id = NodeBinaryOperation,
                        .left = temp_index,
                        .operator = str("."),
                        .right = new_node((Node) { .External = { NodeExternal, .data = str("start") } }),
                    },
                }),
            },
        });

        Node* const data_node = new_node((Node) {
            .BinaryOperation = {
                .id = NodeBinaryOperation,
                .left = new_node((Node) {
                    .Wrapper = { WrapperSurround, .Surround = { lefthand, str("("), str(")") } }
                }),
                .operator = str("+"),
                .right = new_node((Node) {
                    .BinaryOperation = {
                        .id = NodeBinaryOperation,
                        .left = temp_index,
                        .operator = str("."),
                        .right = new_node((Node) { .External = { NodeExternal, .data = str("start") } }),
                    }
                }),
            },
        });

        push(&struct_literal->field_names, str("size"));
        push(&struct_literal->field_values, size_node);
        push(&struct_literal->field_names, str("data"));
        push(&struct_literal->field_values, data_node);

        collector->result_value = (void*) struct_literal;
        collector->type = struct_literal->type;
        collector->trace = trace;
        return (void*) collector;
    }
    close_type(opened_index.actions, 0, 2);

    Node* const override = operator_override(lefthand->type, lefthand, index, str("index"), index->trace, parser);
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
                        .operator = str("+"),
                        .right = index,
                    }
                }),
                .prefix = str("("),
                .postfix = str(")"),
            },
        },
    });

    return dereference(offset, trace, parser->tokenizer->messages);
}
