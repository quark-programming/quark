#include "parser.h"

#include "impl/std.h"

Map(Module) global_modules = NULL;

Scope* new_scope(Declaration* parent);

Parser create_parser(Tokenizer* tokenizer, const str filename, const bool root, str module_identifier) {
    const Module module_data = {
        .id = NodeModule,
        .flags = fConst | fConstExpr,
        .scope = new_scope(NULL),
        .root = root,
    };

    put(&global_modules, filename, module_data);
    Module* const module = get(global_modules, filename);

    static Declaration fake_entry = { NodeEntryFunctionDeclaration };
    static Scope fake_scope = {
        .id = NodeScope,
        .declaration = &fake_entry,
    };

    Declaration* const module_declaration = (void*) new_node((Node) {
        .Declaration = {
            .id = NodeNone,
            .flags = fConst | fConstExpr,
            .type = (void*) module,
            .identifier = {
                .base = module_identifier,
                .parent_declaration = &fake_entry,
                .parent_scope = &fake_scope,
            },
            .const_value = (void*) module,
        },
    });
    module->scope->declaration = module_declaration;

    char* dir_offset = strrchr(filename.data, '/');
    str dir_path = { dir_offset - filename.data, filename.data };
    if(!dir_offset) {
        dir_path = str("./");
    }

    const Parser parser = {
        .tokenizer = tokenizer,
        .module = module,
        .dir_path = dir_path,
        .stack = vec(&global_std_scope, module->scope),
    };

    return parser;
}

Node* eval_w(const char* filename, char* code, Parser* parser, Node* (*supplier)(Parser*)) {
    Tokenizer eval_tokenizer = new_tokenizer(filename
                                             ? : parser->tokenizer->current.trace.filename, code,
                                             parser->tokenizer->messages);
    Tokenizer* const tokenizer = parser->tokenizer;

    parser->tokenizer = &eval_tokenizer;
    Node* const node = supplier(parser);
    parser->tokenizer = tokenizer;

    return node;
}
