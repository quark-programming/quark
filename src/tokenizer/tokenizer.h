#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <helpers.h>
#include "trace.h"

enum {
    TokenIdentifier = 'a',
    TokenNumber,
    TokenString,
    TokenCharacter,
    TokenRightArrow,

    TokenDoubleColon,
    TokenDoublePlus,
    TokenDoubleMinus,
    TokenDoubleLess,
    TokenDoubleGreater,
    TokenDoubleEqual,
    TokenDoubleAnd,
    TokenDoubleOr,

    TokenLessEqual,
    TokenGreaterEqual,
    TokenNotEqual,
    TokenPlusEqual,
    TokenMinusEqual,
    TokenTimesEqual,
    TokenDivideEqual,
    TokenModEqual,
    TokenAndEqual,
    TokenXorEqual,
    TokenOrEqual,
};

extern unsigned char const tokenizer_double_characters[128];
extern unsigned char const tokenizer_equal_characters[128];

struct Parser;
struct Token;

enum {
    KeywordActionNone,
    KeywordActionSelf,
};

typedef struct Keyword {
    unsigned specific_action;
    union Node* (*consumer)(struct Token, struct Parser*);
} Keyword;

typedef struct Token {
    Trace trace;
    unsigned char type;

    union {
        struct {
            Keyword keyword;
            bool is_keyword : 1;
        } identifier;
    };
} Token;

typedef struct Tokenizer {
    Token current;
    MessageVector* messages;
} Tokenizer;

bool char_within_range(char ch, notnull const char* ranges);

Token create_token(Trace trace);

Tokenizer new_tokenizer(notnull const char* filename, notnull char* data, notnull MessageVector* messages);

Token next(notnull Tokenizer* tokenizer);

Token expect(notnull Tokenizer* tokenizer, unsigned char type);

bool try(notnull Tokenizer* tokenizer, unsigned char type, Token* result);

#endif
