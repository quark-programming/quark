#ifndef PARSER_H
#define PARSER_H

#include "nodes/nodes.h"

typedef struct Parser {
    Tokenizer* tokenizer;
    Stack stack;
} Parser;

Node* eval_w(const char* filename, char* code, Parser* parser, Node* (*supplier)(Parser*));

#define eval(filename, code, parser) eval_w(filename, code, parser, &expression)

#endif