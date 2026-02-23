#include "parser.h"

Map(Module) global_modules = NULL;

Scope* new_scope(Declaration* parent);

Parser create_parser(Tokenizer* tokenizer, const str filename) {
    const Module module = {
        .id = NodeModule,
        .scope = new_scope(NULL),
        .root = true,
    };

    put(&global_modules, filename, module);

    const Parser parser = {
        .tokenizer = tokenizer,
        .module = get(global_modules, filename),
        .dir_path = { strrchr(filename.data, '/') - filename.data, filename.data },
    };
    push(&parser.stack, parser.module->scope);

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
