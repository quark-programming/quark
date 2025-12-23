#include "parser.h"

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

