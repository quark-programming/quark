#include "structure.h"

#include "scope.h"
#include "statement.h"
#include "tty.h"
#include "../type/types.h"
#include "../righthand/righthand.h"
#include "../righthand/declaration/declaration.h"
#include "parser/lefthand/lefthand.h"
#include "parser/righthand/declaration/function.h"
#include "parser/righthand/declaration/identifier.h"
#include "parser/righthand/declaration/variable.h"
#include "parser/type/clash_types.h"
#include "parser/type/stringify_type.h"

// TODO: add `Trace trace` argument for info.trace (or `IdentifierInfo info` argument)
Node* parse_struct_literal(Type* const wrapped_struct_type, Parser* parser) {
    const OpenedType opened = open_type(wrapped_struct_type, 0, 2);
    StructType* const struct_type = (void*) opened.type;

    // TODO: error message if not struct
    if(struct_type->id != NodeStructType) {
        push(parser->tokenizer->messages,
             MERROR(wrapped_struct_type->trace, str("creating structure literal with a non-structure type")));

        close_type(opened.actions, 0, 2);
        return (void*) wrapped_struct_type;
    }

    StructLiteral* struct_literal = (void*) new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = (void*) wrapped_struct_type,
        }
    });

    for(u32 field_index = 0; parser->tokenizer->current.type && parser->tokenizer->current.type != '}'; field_index++) {
        str field_name = { 0 };
        StructField* compare_field = struct_type->fields + field_index;

        Token field_name_token;
        if(try(parser->tokenizer, TokenIdentifier, &field_name_token)) {
            if(try(parser->tokenizer, ':', NULL)) {
                field_name = field_name_token.trace.source;

                bool found_compare_field = false;
                for(size_t i = field_index; i < len(struct_type->fields); i++) {
                    if(streq(struct_type->fields[i].identifier, field_name_token.trace.source)) {
                        compare_field = struct_type->fields + i;
                        found_compare_field = true;
                    }
                }

                if(!found_compare_field) {
                    String message = strf(0, iftty("no field named '\33[35m%.*s\33[35m' on '\33[35m",
                                              "no field named '%.*s' on '"),
                                          fmtof(field_name)).as_owned;
                    stringify_type((void*) struct_type, &message, 0, 2);
                    push(parser->tokenizer->messages,
                         MERROR(field_name_token.trace, strf(&message, iftty("\33[0m'", "'"))));
                }
            } else {
                parser->tokenizer->current = field_name_token;
            }
        }

        Node* const field_value = expression(parser);

        if(field_index >= len(struct_type->fields)) {
            push(parser->tokenizer->messages, MERROR(field_value->trace, str("excess fields in struct literal")));
        } else {
            clash_types(compare_field->type, field_value->type, field_value->trace, parser->tokenizer->messages, 0);
        }

        push(&struct_literal->field_names, field_name);
        push(&struct_literal->field_values, field_value);

        if(!try(parser->tokenizer, ',', 0)) break;
    }

    struct_literal->trace = stretch(wrapped_struct_type->trace, expect(parser->tokenizer, '}').trace);
    close_type(opened.actions, 0, 2);
    return (void*) struct_literal;
}

// TODO: actually implement traits u bum
Node* parser_struct_declaration(const Token keyword, Parser* parser, bool is_trait) {
    const Trace trace_start = keyword.trace;
    IdentifierInfo info = new_identifier(expect(parser->tokenizer, TokenIdentifier), parser, IdentifierDeclaration);

    StructType* type = (void*) new_type((Type) {
        .StructType = {
            .id = NodeStructType,
            .flags = fConstExpr,
            .trace = stretch(trace_start, info.trace),
            .is_trait = is_trait,
        }
    });

    Module* module = (void*) new_node((Node) {
        .Module = {
            .id = NodeModule,
            .type = (void*) type,
            .scope = info.generics_collection.generic_declarations_scope,
        },
    });

    type->module = module;
    if(!module->scope) module->scope = new_scope(NULL);

    // TODO: create a flag that only allows type to compile if it is pointed to (in reference()) this will prevent
    //  circular types and allow structs to reference themselves within themselves

    StructDeclaration* declaration = (void*) new_node((Node) {
        .StructDeclaration = {
            .id = NodeStructDeclaration,
            .flags = fConst | fType,
            .trace = type->trace,
            .type = (void*) type,
            .const_value = (void*) type,
            .identifier = info.identifier,
        }
    });

    module->scope->declaration = (void*) declaration;
    module->declaration = (void*) declaration;
    put(&info.declaration_scope->variables, info.identifier.base, (void*) declaration);
    assign_generics_to_declaration((void*) declaration, info.generics_collection);
    declaration->identifier.parent_declaration = (void*) declaration;

    push(&parser->stack, module->scope);
    expect(parser->tokenizer, '{');

    while(parser->tokenizer->current.type && parser->tokenizer->current.type != '}') {
        Token static_token = { 0 };
        if(parser->tokenizer->current.type == TokenIdentifier
           && streq(parser->tokenizer->current.trace.source, str("static"))) {
            static_token = next(parser->tokenizer);
        }

        Type* const declaration_type = (void*) righthand_expression(lefthand_expression(parser), parser, 11);
        if(!(declaration_type->flags & fType)) {
            push(parser->tokenizer->messages, MERROR(declaration_type->trace,
                     str("expected a declaration in body of structure")));
            continue;
        }

        const IdentifierInfo declaration_info = new_identifier(expect(parser->tokenizer, TokenIdentifier), parser,
                                                               IdentifierDeclaration);
        if(try(parser->tokenizer, '(', NULL)) {
            if(static_token.type) {
                push(parser->tokenizer->messages, MWARN(static_token.trace,
                         str("static is ignored before function declaration")));
                push(parser->tokenizer->messages,
                     MHINT(str("methods are always able to be called statically using '::' syntax")));
            }

            unbox(parse_function_declaration(declaration_type, declaration_info, parser));
            continue;
        }

        if(static_token.type) {
            Wrapper* const variable = (void*) parse_variable_declaration(declaration_type, declaration_info, parser);

            if(try(parser->tokenizer, '=', NULL)) {
                Node* const value = expression(parser);
                clash_types(variable->type, value->type, stretch(variable->trace, value->trace),
                            parser->tokenizer->messages, 0);

                variable->Variable.declaration->VariableDeclaration.static_value = value;
            } else {
                variable->Variable.declaration->VariableDeclaration.static_value = new_node((Node) { NodeNone });
            }

            expect(parser->tokenizer, ';');
            continue;
        }

        push(&type->fields, { declaration_type, declaration_info.trace.source });
        expect(parser->tokenizer, ';');
    }

    // Node* next_declaration = 0;
    // // TODO: make the `NodeNone` with `.type` system more readable
    // while(parser->tokenizer->current.type && parser->tokenizer->current.type != '}'
    //       && !(next_declaration = statement(parser))->id && next_declaration->type) {
    //     if(next_declaration->type->id != WrapperVariable || !(next_declaration->type->flags & fIgnoreStatement)
    //        || next_declaration->type->Wrapper.Variable.declaration->id != NodeVariableDeclaration)
    //         continue;
    //
    //     VariableDeclaration* const field_decl = (void*) next_declaration->type->Wrapper.Variable.declaration;
    //     const StructField field = {
    //         .type = field_decl->type,
    //         .identifier = field_decl->identifier.base,
    //     };
    //
    //     push(&type->fields, field);
    //
    //     unbox((void*) field_decl);
    //     unbox((void*) next_declaration->type);
    //     unbox(next_declaration);
    // }
    //
    // Vec(Node*) declarations = { 0 };
    // if(!try(parser->tokenizer, '}', NULL)) {
    //     push(&declarations, next_declaration);
    //
    //     while(parser->tokenizer->current.type && !try(parser->tokenizer, '}', NULL)) {
    //         push(&declarations, statement(parser));
    //     }
    // }

    expect(parser->tokenizer, '}');
    pop(&parser->stack);
    close_generics_declaration((void*) declaration);

    return new_node((Node) { NodeNone });
}
