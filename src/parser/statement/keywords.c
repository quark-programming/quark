#include "keywords.h"

#include "modules.h"
#include "scope.h"
#include "statement.h"
#include "../righthand/righthand.h"
#include "../righthand/declaration/identifier.h"
#include "../type/clash_types.h"
#include "../type/types.h"

Vec(str) global_library_paths = NULL;

// TODO: (organizational) move some of these functions out of this file

Node* keyword_import(const Token token, Parser* parser) {
    Trace trace = token.trace;
    ModuleExtension extension = ExtensionNone;
    str import_identifier = { 0 };

    const String import_path = build_import_path(&trace, parser, &extension, &import_identifier);
    Parser imported_parser = find_import(import_path, trace, parser);
    free(vbase(import_path));

    if(!imported_parser.module) {
        return new_node((Node) { NodeNone });
    }

    Vec(Node*) const import_body = imported_parser.tokenizer ? collect_until(&imported_parser, &statement, 0, 0) : NULL;

    switch(extension) {
        case ExtensionNone: {
            imported_parser.module->flags |= fConst | fConstExpr;
            Declaration* const module_declaration = (void*) new_node((Node) {
                .Declaration = {
                    .id = NodeNone,
                    .flags = fConst | fConstExpr,
                    .type = (void*) imported_parser.module,
                    .const_value = (void*) imported_parser.module,
                },
            });
            put(&last(parser->stack)->variables, import_identifier, module_declaration);
            break;
        }

        default: panicf("i haven't done this yet");
    }

    return new_node((Node) {
        .Scope = {
            .id = NodeScope,
            .children = import_body,
        }
    });
}

Node* keyword_return(const Token token, Parser* parser) {
    const Trace trace_start = token.trace;
    Node* value = parser->tokenizer->current.type == ';' ? NULL : expression(parser);
    expect(parser->tokenizer, ';');

    if(last(parser->stack)->declaration->id != NodeFunctionDeclaration) {
        push(parser->tokenizer->messages,
             MERROR(value ? stretch(trace_start, value->trace) : trace_start,
                 str("return statement needs to be inside of a function")));
    } else if(value) {
        clash_types(last(parser->stack)->declaration->FunctionDeclaration.type->FunctionType.signature[0], value->type,
                    value->trace, parser->tokenizer->messages, 0);
    }

    return new_node((Node) {
        .ReturnStatement = {
            .id = NodeReturnStatement,
            .value = value,
        }
    });
}

Node* keyword_struct(const Token token, Parser* parser) {
    const Trace trace_start = token.trace;
    IdentifierInfo info = new_identifier(expect(parser->tokenizer, TokenIdentifier), parser, IdentifierDeclaration);

    StructType* type = (void*) new_type((Type) {
        .StructType = {
            .id = NodeStructType,
            .flags = fConstExpr,
            .trace = stretch(trace_start, info.trace),
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

Node* keywords_control(const Token keyword, Parser* parser) {
    expect(parser->tokenizer, '(');

    Vec(Node*) const conditions = collect_until(parser, &expression, ';', ')');
    if(len(conditions) != (keyword.identifier.keyword.specific_action - KeywordControlSingleCond) * 2 + 1) {
        push(parser->tokenizer->messages, MERROR(stretch(conditions[0]->trace, last(conditions)->trace),
                 str("too many or too little conditions (separated by ';') in control statement")));
    }

    Node* body_node = statement(parser);
    Scope* body;

    if(body_node->id == NodeScope) {
        body = (void*) body_node;
    } else {
        body = new_scope(last(parser->stack)->declaration);
        push(&body->children, body_node);
    }

    return new_node((Node) {
        .ControlStatement = {
            .id = NodeControlStatement,
            .keyword = keyword.trace.source,
            .conditions = conditions,
            .body = body,
        }
    });
}

Node* keyword_type(Token token, Parser* parser) {
    (void) token;

    const IdentifierInfo info = new_identifier(expect(parser->tokenizer, TokenIdentifier), parser,
                                               IdentifierDeclaration);

    expect(parser->tokenizer, '=');
    Type* const type = (void*) expression(parser);
    type->flags |= fConstExpr;
    expect(parser->tokenizer, ';');

    Declaration* const declaration = (void*) new_node((Node) {
        .VariableDeclaration = {
            .id = NodeVariableDeclaration,
            .trace = info.trace,
            .type = type,
            .flags = fType | fConst,
            .const_value = (void*) type,
        }
    });

    put(&info.declaration_scope->variables, info.identifier.base, declaration);
    return new_node((Node) { NodeNone });
}
