#ifndef LITERAL_STRING_H
#define LITERAL_STRING_H

#include "../nodes/nodes.h"
#include "../parser.h"

size_t calculate_string_length(const char* string_literal, size_t length);

Node* string_literal(Token token, Parser* parser);

Node* character_literal(Token token, Parser* parser);

#endif