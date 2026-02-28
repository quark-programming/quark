#include "keywords.h"
#include "../type/types.h"
#include "../righthand/righthand.h"
#include "../statement/scope.h"
#include "lefthand.h"
#include "tty.h"
#include "../literal/wrapper.h"

// TODO: (organizational) move some of these to parser/literal and move parser/righthand/declaration
//  into parser/declaration

Node* keyword_auto(const Token token, Parser* parser) {
    (void) parser;

    return (void*) new_type((Type) {
        .Wrapper = {
            .id = WrapperAuto,
            .trace = token.trace,
        }
    });
}

Node* keyword_int(const Token token, Parser* parser) {
    (void) parser;

    return (void*) new_type((Type) {
        .Wrapper = {
            .id = WrapperAuto,
            .flags = fNumeric,
            .trace = token.trace,
        }
    });
}

Node* keyword_typeof(const Token token, Parser* parser) {
    // TODO: (typeof) fix trace
    (void) token;

    expect(parser->tokenizer, '(');
    Node* const argument = expression(parser);
    expect(parser->tokenizer, ')');
    return (void*) argument->type;
}

Node* keyword_sizeof(const Token token, Parser* parser) {
    expect(parser->tokenizer, '(');

    static Type usize_type = {
        .External = {
            .id = NodeExternal,
            .flags = fNumeric | fType,
            .type = &usize_type,
            .data = str("size_t"),
        },
    };

    Vec(Node*) const arguments = vec(expression(parser));

    return new_node((Node) {
        .FunctionCall = {
            .id = NodeFunctionCall,
            .trace = stretch(token.trace, expect(parser->tokenizer, ')').trace),
            .function = new_node((Node) {
                .External = {
                    .id = NodeExternal,
                    .data = str("sizeof"),
                }
            }),
            .arguments = arguments,
            .type = &usize_type,
        }
    });
}

Node* keyword_const(const Token token, Parser* parser) {
    // TODO: (const) fix trace
    (void) token;

    Type* type = get_type(parser);
    type->flags |= fConst;
    return (void*) type;
}

Node* keyword_extern(const Token token, Parser* parser) {
    Type* type = NULL;
    unsigned long flags = 0;

    if(try(parser->tokenizer, '<', 0)) {
        // TODO: maybe move to a flag in `Parser`
        global_righthand_collecting_type_arguments = true;
        type = get_type(parser);
        global_righthand_collecting_type_arguments = false;

        expect(parser->tokenizer, '>');
    } else {
        flags |= fType;
        if(streq(parser->tokenizer->current.trace.source, str("int"))) {
            flags |= fNumeric;
            next(parser->tokenizer);
        }
    }

    str data;
    Token data_token;

    if(try(parser->tokenizer, TokenString, &data_token)) {
        data = (str) { data_token.trace.source.len - 2, data_token.trace.source.data + 1 };
    } else if(parser->tokenizer->current.type == '{') {
        parser->tokenizer->remove_newlines = false;
        Token data_end = data_token = next(parser->tokenizer);

        for(u32 depth = 1; parser->tokenizer->current.type && depth;) {
            data_end = next(parser->tokenizer);
            if(data_end.type == '{') depth++;
            else if(data_end.type == '}') depth--;
        }
        parser->tokenizer->remove_newlines = true;

        data = (str) {
            .len = data_end.trace.source.data - data_token.trace.source.data,
            .data = data_token.trace.source.data + 1,
        };
        if(data.len) data.len -= 2;
        data_token = data_end;
    } else {
        data_token = expect(parser->tokenizer, TokenIdentifier);
        data = data_token.trace.source;
    }

    Node* external = new_node((Node) {
        .External = {
            .id = NodeExternal,
            .flags = fConstExpr | flags,
            .trace = stretch(token.trace, data_token.trace),
            .type = type,
            .data = data,
        }
    });

    if(!external->type) {
        external->type = (void*) external;
    }

    return external;
}

Node* keyword_private(Token token, Parser* parser) {
    (void) token;

    Wrapper* const wrapper = (void*) expression(parser);
    if(wrapper->id != WrapperVariable) {
        push(parser->tokenizer->messages, MERROR(wrapper->trace, str("used private on a non-variable value")));
        return (void*) wrapper;
    }

    wrapper->Variable.declaration->flags |= fPrivate;
    return (void*) wrapper;
}
