#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <wrap.h>
#include "trace.h"

enum {
    TokenIdentifier = 'a',
    TokenNumber,
    TokenString,
    TokenCharacter,
    TokenRightArrow,
    TokenDoubleRightArrow,

    TokenDoubleColon,
    TokenDoublePlus,
    TokenDoubleMinus,
    TokenDoubleLess,
    TokenDoubleGreater,
    TokenDoubleEqual,
    TokenDoubleAnd,
    TokenDoubleOr,
    TokenDoubleDot,

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
    TokenOrEqual = 'z',
};

extern unsigned char const tokenizer_double_characters[];
extern unsigned char const tokenizer_equal_characters[];

extern char* const token_type_strings[];

char* token_type_to_string(char type);

struct Parser;
struct Token;

enum {
    KeywordActionNone,
    // KeywordActionSelf,

    KeywordActionStatement = 1 << 2,
    KeywordControlSingleCond,
    KeywordControlTripleCond,
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
            bool is_keyword         : 1,
                    searched_keyword: 1;
        } identifier;
    };
} Token;

typedef struct Tokenizer {
    Token current;
    Vec(Message)* messages;
    bool remove_newlines;
} Tokenizer;

bool char_within_range(char ch, const char* ranges);

Token create_token(Trace trace, bool remove_newlines);

Tokenizer new_tokenizer(const char* filename, char* data, Vec(Message)* messages);

Token next(Tokenizer* tokenizer);

Token expect(Tokenizer* tokenizer, unsigned char type);

bool try(Tokenizer* tokenizer, unsigned char type, Token* result);

#endif
