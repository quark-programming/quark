#include "tokenizer.h"

#include "tty.h"

unsigned char const tokenizer_double_characters[128] = {
    [':'] = TokenDoubleColon, ['+'] = TokenDoublePlus, ['-'] = TokenDoubleMinus,
    ['<'] = TokenDoubleLess, ['>'] = TokenDoubleGreater, ['='] = TokenDoubleEqual,
    ['&'] = TokenDoubleAnd, ['|'] = TokenDoubleOr, ['.'] = TokenDoubleDot,
};

unsigned char const tokenizer_equal_characters[128] = {
    ['<'] = TokenLessEqual, ['>'] = TokenGreaterEqual, ['!'] = TokenNotEqual,
    ['+'] = TokenPlusEqual, ['-'] = TokenMinusEqual, ['*'] = TokenTimesEqual,
    ['/'] = TokenDivideEqual, ['%'] = TokenModEqual, ['&'] = TokenAndEqual,
    ['^'] = TokenXorEqual, ['|'] = TokenOrEqual,
};

char* const token_type_strings[128] = {
    [TokenIdentifier] = "identifier",
    [TokenNumber] = "number",
    [TokenString] = "string",
    [TokenCharacter] = "character",
    [TokenRightArrow] = "->",
    [TokenDoubleRightArrow] = "=>",

    [TokenDoubleColon] = "::",
    [TokenDoublePlus] = "++",
    [TokenDoubleMinus] = "--",
    [TokenDoubleLess] = "<<",
    [TokenDoubleGreater] = ">>",
    [TokenDoubleEqual] = "==",
    [TokenDoubleAnd] = "&&",
    [TokenDoubleOr] = "||",
    [TokenDoubleDot] = "..",

    [TokenLessEqual] = "<=",
    [TokenGreaterEqual] = ">=",
    [TokenNotEqual] = "!=",
    [TokenPlusEqual] = "+=",
    [TokenMinusEqual] = "-=",
    [TokenTimesEqual] = "*=",
    [TokenDivideEqual] = "/=",
    [TokenModEqual] = "%=",
    [TokenAndEqual] = "&=",
    [TokenXorEqual] = "^=",
    [TokenOrEqual] = "|=",
};

char* token_type_to_string(const char type) {
    if(token_type_strings[type]) return token_type_strings[type];

    static char buffer[2] = { 0 };
    buffer[0] = type;
    return buffer;
}

static bool char_within_ranges(const char ch, const char* ranges) {
    for(; *ranges; ranges += 2)
        if(ch >= ranges[0] && ch <= ranges[1]) return 1;
    return 0;
}

Token create_token(Trace trace, const bool remove_newlines) {
    trace.source.data += trace.source.len;
    trace.source.len = 0;

    while(*trace.source.data && *trace.source.data <= ' ') {
        if(*trace.source.data++ == '\n') {
            if(remove_newlines) trace.source.data[-1] = 0;
            trace.col = 0;
            trace.row++;
            trace.line_start = trace.source.data;
        }
        trace.col++;
    }

    if(char_within_ranges(*trace.source.data, "azAZ__")) {
        while(char_within_ranges(trace.source.data[++trace.source.len], "azAZ__09"));
        trace.col += trace.source.len;
        return (Token) { trace, TokenIdentifier };
    }

    if(char_within_ranges(*trace.source.data, "09")) {
        while(char_within_ranges(trace.source.data[++trace.source.len], "09"));
        trace.col += trace.source.len;
        return (Token) { trace, TokenNumber };
    }

    if(*trace.source.data == '"' || *trace.source.data == '\'') {
        while(trace.source.data[++trace.source.len] != trace.source.data[0]) {
            if(trace.source.data[trace.source.len] == '\\')
                trace.source.len++;
        }
        trace.col += ++trace.source.len;
        return (Token) { trace, trace.source.data[0] == '"' ? TokenString : TokenCharacter };
    }

    if(tokenizer_double_characters[*trace.source.data] &&
        trace.source.data[0] == trace.source.data[1]) {
        trace.col += trace.source.len = 2;
        return (Token) { trace, tokenizer_double_characters[*trace.source.data] };
    }

    if(tokenizer_equal_characters[*trace.source.data] && trace.source.data[1] == '=') {
        trace.col += trace.source.len = 2;
        return (Token) { trace, tokenizer_equal_characters[*trace.source.data] };
    }

    if(trace.source.data[0] == '-' && trace.source.data[1] == '>') {
        trace.col += trace.source.len = 2;
        return (Token) { trace, TokenRightArrow };
    }

    if(trace.source.data[0] == '=' && trace.source.data[1] == '>') {
        trace.col += trace.source.len = 2;
        return (Token) { trace, TokenDoubleRightArrow };
    }

    if(trace.source.data[0] == '/' && trace.source.data[1] == '/') {
        while(*++trace.source.data && *trace.source.data != '\n');
        return create_token(trace, remove_newlines);
    }

    if(trace.source.data[0] == '/' && trace.source.data[1] == '*') {
        while((*++trace.source.data && *trace.source.data != '*') || *++trace.source.data != '/');
        trace.source.data++;
        return create_token(trace, remove_newlines);
    }

    trace.col += trace.source.len = 1;
    return (Token) { trace, *trace.source.data };
}

Tokenizer new_tokenizer(const char* const filename, char* const data, Vec(Message)* const messages) {
    return (Tokenizer) {
        .current = create_token((Trace) {
                                    .source = { 0, data },
                                    .filename = filename,
                                    .line_start = data,
                                    .col = 1, .row = 1,
                                }, true),
        .messages = messages,
        .remove_newlines = true,
    };
}

Token next(Tokenizer* const tokenizer) {
    const Token next = tokenizer->current;

    if(!next.type) {
        push(tokenizer->messages, MERROR(next.trace, str(
                 iftty("expected a token, but got "HERR"end of file"H,
                     "expected a token, but got end of file" ))));
    } else {
        tokenizer->current = create_token(next.trace, tokenizer->remove_newlines);
    }

    return next;
}

Token expect(Tokenizer* const tokenizer, const unsigned char type) {
    const Token expect = next(tokenizer);

    if(expect.type != type) {
        push(tokenizer->messages, MERROR(expect.trace,
                 strf(0, iftty("expected "HERR"%s"H", but got "HERR"%.*s"H, "expected %s, but got %.*s"),
                     token_type_to_string(type), fmtof(expect.trace.source))));
    }

    return expect;
}

bool try(Tokenizer* const tokenizer, const unsigned char type, Token* result) {
    if(tokenizer->current.type != type) return false;
    const Token try = next(tokenizer);
    if(result) *result = try;
    return true;
}
