#ifndef PARSER_H
#define PARSER_H

#include "fs.h"
#include "nodes/nodes.h"

typedef struct Parser {
    Tokenizer* tokenizer;
    Stack stack;
    Module* module;
    str dir_path;
} Parser;

extern Map(Module*) global_modules;

Parser create_parser(Tokenizer* tokenizer, str filename, bool root, str module_identifier);

Node* eval_w(const char* filename, char* code, Parser* parser, Node* (*supplier)(Parser*));

#define eval(filename, code, parser) eval_w(filename, code, parser, &expression)

#endif