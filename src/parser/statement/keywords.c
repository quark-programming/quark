#include "keywords.h"

#include "scope.h"
#include "statement.h"
#include "../righthand/righthand.h"
#include "../righthand/declaration/identifier.h"
#include "../type/clash_types.h"
#include "../type/types.h"

Vec(char*) global_library_paths = NULL;

// TODO: (organizational) move some of these functions out of this file

Node* keyword_import(const Token token, Parser* parser) {
    String sub_path = NULL;
    Trace full_trace = token.trace;

    do {
        const Trace section = expect(parser->tokenizer, TokenIdentifier).trace;
        strf(&sub_path, "/%.*s", fmtof(section.source));
        full_trace = stretch(full_trace, section);
    } while(try(parser->tokenizer, TokenDoubleColon, NULL));
    expect(parser->tokenizer, ';');

    strf(&sub_path, ".qk");

    String import_path = NULL;
    for(size_t i = 0; i < len(global_library_paths); i++) {
        strf(&import_path, "%s%.*s%c", global_library_paths[i], fmtof(sub_path), 0);
        char* input_content = fs_readfile(import_path);

        if(!input_content) {
            len(import_path) = 0;
            continue;
        }

        Tokenizer import_tokenizer = new_tokenizer(import_path, input_content, parser->tokenizer->messages);
        Tokenizer* const tokenizer = parser->tokenizer;
        parser->tokenizer = &import_tokenizer;

        Scope* scope = new_scope(NULL);
        scope->children = collect_until(parser, &statement, 0, 0);
        parser->tokenizer = tokenizer;
        return (void*) scope;
    }

    push(parser->tokenizer->messages,
         MERROR(full_trace, strf(0, "unable to open or read '%.*s'", fmtof(import_path))));
    return new_node((Node) { NodeNone });
}

Node* keyword_return(const Token token, Parser* parser) {
    const Trace trace_start = token.trace;
    Node* value = parser->tokenizer->current.type == ';' ? NULL : expression(parser);
    expect(parser->tokenizer, ';');

    if(last(parser->stack)->parent->id != NodeFunctionDeclaration) {
        push(parser->tokenizer->messages,
             MERROR(value ? stretch(trace_start, value->trace) : trace_start,
                 str("return statement needs to be inside of a function")));
    } else if(value) {
        clash_types(last(parser->stack)->parent->FunctionDeclaration.type->FunctionType.signature[0], value->type,
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
    module->scope->parent = (void*) type;

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

    put(&info.declaration_scope->variables, info.identifier.base, (void*) declaration);
    assign_generics_to_declaration((void*) declaration, info.generics_collection);
    declaration->identifier.parent_declaration = (void*) declaration;
    module->declaration = (void*) declaration;

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
        body = new_scope(last(parser->stack)->parent);
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
