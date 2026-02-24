#include "structure.h"

#include "scope.h"
#include "statement.h"
#include "tty.h"
#include "../type/types.h"
#include "../righthand/righthand.h"
#include "../righthand/declaration/declaration.h"
#include "parser/righthand/declaration/identifier.h"
#include "parser/type/stringify_type.h"

// TODO: add `Trace trace` argument for info.trace (or `IdentifierInfo info` argument)
Node* parse_struct_literal(Type* const wrapped_struct_type, Parser* parser) {
    const OpenedType opened = open_type(wrapped_struct_type, 0);
    StructType* const struct_type = (void*) opened.type;

    // TODO: error message if not struct
    if(struct_type->id != NodeStructType) {
        push(parser->tokenizer->messages,
             MERROR(wrapped_struct_type->trace, str("creating structure literal with a non-structure type")));

        close_type(opened.actions, 0);
        return (void*) wrapped_struct_type;
    }

    StructLiteral* struct_literal = (void*) new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = (void*) wrapped_struct_type,
        }
    });

    while(parser->tokenizer->current.type && parser->tokenizer->current.type != '}') {
        if(parser->tokenizer->current.type == TokenIdentifier) {
            const Token field_name = next(parser->tokenizer);

            if(try(parser->tokenizer, ':', NULL)) {
                push(&struct_literal->field_names, field_name.trace.source);
                push(&struct_literal->field_values, expression(parser));

                for(size_t i = 0; i < len(struct_type->fields); i++) {
                    if(streq(struct_type->fields[i].identifier, field_name.trace.source)) {
                        goto continue_;
                    }
                }

                String message = strf(0, iftty("no field named '\33[35m%.*s\33[35m' on '\33[35m",
                                          "no field named '%.*s' on '")).as_owned;
                stringify_type((void*) struct_type, &message, 0);
                push(parser->tokenizer->messages, MERROR(field_name.trace, strf(&message, iftty("\33[0m'", "'"))));

            continue_:
                if(!try(parser->tokenizer, ',', 0)) break;
                continue;
            }

            parser->tokenizer->current = field_name;
        }

        push(&struct_literal->field_names, { 0 });
        push(&struct_literal->field_values, expression(parser));

        if(!try(parser->tokenizer, ',', 0)) break;
    }

    struct_literal->trace = stretch(wrapped_struct_type->trace, expect(parser->tokenizer, '}').trace);
    close_type(opened.actions, 0);
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

    Node* next_declaration = 0;
    // TODO: make the `NodeNone` with `.type` system more readable
    while(parser->tokenizer->current.type && parser->tokenizer->current.type != '}'
          && !(next_declaration = statement(parser))->id && next_declaration->type) {
        if(next_declaration->type->id != WrapperVariable || !(next_declaration->type->flags & fIgnoreStatement)
           || next_declaration->type->Wrapper.Variable.declaration->id != NodeVariableDeclaration)
            break;

        VariableDeclaration* const field_decl = (void*) next_declaration->type->Wrapper.Variable.declaration;
        const StructField field = {
            .type = field_decl->type,
            .identifier = field_decl->identifier.base,
        };

        push(&type->fields, field);

        unbox((void*) field_decl);
        unbox((void*) next_declaration->type);
        unbox(next_declaration);
    }

    Vec(Node*) declarations = { 0 };
    if(!try(parser->tokenizer, '}', NULL)) {
        push(&declarations, next_declaration);

        while(parser->tokenizer->current.type && !try(parser->tokenizer, '}', NULL)) {
            push(&declarations, statement(parser));
        }
    }

    pop(&parser->stack);
    close_generics_declaration((void*) declaration);
    module->scope->children = declarations;

    return new_node((Node) { NodeNone });
}
