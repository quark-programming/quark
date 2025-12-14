#ifndef PARSER_H
#define PARSER_H

#include "nodes/nodes.h"

typedef struct Parser {
    Tokenizer* tokenizer;
    Stack stack;
} Parser;

#endif