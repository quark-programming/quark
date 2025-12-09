#include "trace.c"

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

typedef struct {
    Trace trace;
    unsigned char type;
} Token;

typedef struct {
    Token current;
    Message_Vector* messages;
} Tokenizer;

int char_ranges(char ch, char* ranges) {
    for (; *ranges; ranges += 2)
        if (ch >= ranges[0] && ch <= ranges[1]) return 1;
    return 0;
}

Token create_token(Trace trace) {
    trace.slice.data += trace.slice.size;
    trace.slice.size = 0;

    while (*trace.slice.data && *trace.slice.data <= ' ')
    {
        if (*trace.slice.data++ == '\n')
        {
            trace.slice.data[-1] = 0;
            trace.col = 0;
            trace.row++;
            trace.line_start = trace.slice.data;
        }
        trace.col++;
    }

    if (char_ranges(*trace.slice.data, "azAZ__"))
    {
        while (char_ranges(trace.slice.data[++trace.slice.size],
                           "azAZ__09"));
        trace.col += trace.slice.size;
        return (Token){trace, TokenIdentifier};
    }

    if (char_ranges(*trace.slice.data, "09"))
    {
        while (char_ranges(trace.slice.data[++trace.slice.size],
                           "09"));
        trace.col += trace.slice.size;
        return (Token){trace, TokenNumber};
    }

    if (*trace.slice.data == '"' || *trace.slice.data == '\'')
    {
        while (trace.slice.data[++trace.slice.size] != trace.slice.data[0])
        {
            if (trace.slice.data[trace.slice.size] == '\\')
                trace.slice.size++;
        }
        trace.col += ++trace.slice.size;
        return (Token){trace, trace.slice.data[0] == '"' ? TokenString : TokenCharacter};
    }

    if (tokenizer_double_characters[*trace.slice.data] &&
        trace.slice.data[0] == trace.slice.data[1])
    {
        trace.col += trace.slice.size = 2;
        return (Token){trace, tokenizer_double_characters[*trace.slice.data]};
    }

    if (tokenizer_equal_characters[*trace.slice.data] && trace.slice.data[1] == '=')
    {
        trace.col += trace.slice.size = 2;
        return (Token){trace, tokenizer_equal_characters[*trace.slice.data]};
    }

    if (trace.slice.data[0] == '-' && trace.slice.data[1] == '>')
    {
        trace.col += trace.slice.size = 2;
        return (Token){trace, TokenRightArrow};
    }

    if (trace.slice.data[0] == '/' && trace.slice.data[1] == '/')
    {
        while (*++trace.slice.data && *trace.slice.data != '\n');
        return create_token(trace);
    }

    if (trace.slice.data[0] == '/' && trace.slice.data[1] == '*')
    {
        while ((*++trace.slice.data && *trace.slice.data != '*') || *++trace.slice.data != '/');
        trace.slice.data++;
        return create_token(trace);
    }

    trace.col += trace.slice.size = 1;
    return (Token){trace, *trace.slice.data};
}

Tokenizer new_tokenizer(char* filename, char* data, Message_Vector* messages) {
    return (Tokenizer){
        .current = create_token((Trace){
            .slice = {0, 0, data},
            .filename = filename,
            .line_start = data,
            .col = 1, .row = 1,
        }),
        .messages = messages,
    };
}

Token next(Tokenizer* tokenizer) {
    Token next = tokenizer->current;
    if (!next.type)
        push(tokenizer->messages, REPORT_ERR(next.trace,
             str("expected a token, but got \33[35mend of file\33[0m")));
    else tokenizer->current = create_token(next.trace);
    return next;
}

Token expect(Tokenizer* tokenizer, unsigned char type) {
    Token expect = next(tokenizer);
    if (expect.type != type)
        push(tokenizer->messages, REPORT_ERR(expect.trace,
             strf(0, "expected type [\33[35m%c (%u)\33[0m], but got '\33[35m%.*s\33[0m'",
                 type, type, (int) expect.trace.slice.size, expect.trace.slice.data)));
    return expect;
}

int try(Tokenizer* tokenizer, unsigned char type, Token* result) {
    if (tokenizer->current.type != type) return 0;
    Token try = next(tokenizer);
    if (result) *result = try;
    return 1;
}
