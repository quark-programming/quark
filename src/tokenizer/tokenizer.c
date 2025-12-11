#include "tokenizer.h"
#include <vector.h>

unsigned char const tokenizer_double_characters[128] = {
    [':'] = TokenDoubleColon, ['+'] = TokenDoublePlus, ['-'] = TokenDoubleMinus,
    ['<'] = TokenDoubleLess, ['>'] = TokenDoubleGreater, ['='] = TokenDoubleEqual,
    ['&'] = TokenDoubleAnd, ['|'] = TokenDoubleOr,
};

unsigned char const tokenizer_equal_characters[128] = {
    ['<'] = TokenLessEqual, ['>'] = TokenGreaterEqual, ['!'] = TokenNotEqual,
    ['+'] = TokenPlusEqual, ['-'] = TokenMinusEqual, ['*'] = TokenTimesEqual,
    ['/'] = TokenDivideEqual, ['%'] = TokenModEqual, ['&'] = TokenAndEqual,
    ['^'] = TokenXorEqual, ['|'] = TokenOrEqual,
};

bool char_within_range(const char ch, const char* ranges) {
    for(; *ranges; ranges += 2)
        if(ch >= ranges[0] && ch <= ranges[1]) return 1;
    return 0;
}

Token create_token(Trace trace) {
    trace.source.data += trace.source.size;
    trace.source.size = 0;

    while(*trace.source.data && *trace.source.data <= ' ') {
        if(*trace.source.data++ == '\n') {
            trace.source.data[-1] = 0;
            trace.col = 0;
            trace.row++;
            trace.line_start = trace.source.data;
        }
        trace.col++;
    }

    if(char_within_range(*trace.source.data, "azAZ__")) {
        while(char_within_range(trace.source.data[++trace.source.size], "azAZ__09"));
        trace.col += trace.source.size;
        return (Token) { trace, TokenIdentifier };
    }

    if(char_within_range(*trace.source.data, "09")) {
        while(char_within_range(trace.source.data[++trace.source.size],
                                "09"));
        trace.col += trace.source.size;
        return (Token) { trace, TokenNumber };
    }

    if(*trace.source.data == '"' || *trace.source.data == '\'') {
        while(trace.source.data[++trace.source.size] != trace.source.data[0]) {
            if(trace.source.data[trace.source.size] == '\\')
                trace.source.size++;
        }
        trace.col += ++trace.source.size;
        return (Token) { trace, trace.source.data[0] == '"' ? TokenString : TokenCharacter };
    }

    if(tokenizer_double_characters[*trace.source.data] &&
       trace.source.data[0] == trace.source.data[1]) {
        trace.col += trace.source.size = 2;
        return (Token) { trace, tokenizer_double_characters[*trace.source.data] };
    }

    if(tokenizer_equal_characters[*trace.source.data] && trace.source.data[1] == '=') {
        trace.col += trace.source.size = 2;
        return (Token) { trace, tokenizer_equal_characters[*trace.source.data] };
    }

    if(trace.source.data[0] == '-' && trace.source.data[1] == '>') {
        trace.col += trace.source.size = 2;
        return (Token) { trace, TokenRightArrow };
    }

    if(trace.source.data[0] == '/' && trace.source.data[1] == '/') {
        while(*++trace.source.data && *trace.source.data != '\n');
        return create_token(trace);
    }

    if(trace.source.data[0] == '/' && trace.source.data[1] == '*') {
        while((*++trace.source.data && *trace.source.data != '*') || *++trace.source.data != '/');
        trace.source.data++;
        return create_token(trace);
    }

    trace.col += trace.source.size = 1;
    return (Token) { trace, *trace.source.data };
}

Tokenizer new_tokenizer(const char* const filename, char* const data, MessageVector* const messages) {
    return (Tokenizer) {
        .current = create_token((Trace) {
            .source = { 0, 0, data },
            .filename = filename,
            .line_start = data,
            .col = 1, .row = 1,
        }),
        .messages = messages,
    };
}

Token next(Tokenizer* const tokenizer) {
    const Token next = tokenizer->current;

    if(!next.type) {
        push(tokenizer->messages, REPORT_ERR(next.trace, String("expected a token, but got \33[35mend of file\33[0m")));
    } else {
        tokenizer->current = create_token(next.trace);
    }

    return next;
}

Token expect(Tokenizer* const tokenizer, const unsigned char type) {
    const Token expect = next(tokenizer);

    if(expect.type != type) {
        push(tokenizer->messages, REPORT_ERR(expect.trace,
                 strf(0, "expected type [\33[35m%c (%u)\33[0m], but got '\33[35m%.*s\33[0m'",
                     type, type, (int) expect.trace.source.size, expect.trace.source.data)));
    }

    return expect;
}

bool try(Tokenizer* const tokenizer, const unsigned char type, Token* result) {
    if(tokenizer->current.type != type) return false;
    const Token try = next(tokenizer);
    if(result) *result = try;
    return true;
}
