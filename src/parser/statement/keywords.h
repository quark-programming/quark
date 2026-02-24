#ifndef STATEMENT_KEYWORDS_H
#define STATEMENT_KEYWORDS_H

#include "../parser.h"

Node* keyword_import(Token token, Parser* parser);

Node* keyword_return(Token token, Parser* parser);

Node* keyword_struct(Token token, Parser* parser);

Node* keyword_trait(Token token, Parser* parser);

Node* keywords_control(Token keyword, Parser* parser);

Node* keyword_type(Token token, Parser* parser);

#endif