#ifndef LEFTHAND_KEYWORDS_H
#define LEFTHAND_KEYWORDS_H

#include "../nodes/nodes.h"
#include "../parser.h"

Node* keyword_auto(Token token, Parser* parser);

Node* keyword_int(Token token, Parser* parser);

Node* keyword_typeof(Token token, Parser* parser);

Node* keyword_sizeof(Token token, Parser* parser);

Node* keyword_const(Token token, Parser* parser);

Node* keyword_extern(Token token, Parser* parser);

Node* keyword_self(Token token, Parser* parser);

#endif