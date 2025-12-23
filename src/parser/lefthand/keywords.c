#include "keywords.h"
#include "../type/types.h"
#include "../righthand/righthand.h"
#include "../statement/scope.h"
#include "lefthand.h"
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
    NodeVector arguments = { 0 };
    push(&arguments, expression(parser));

    return new_node((Node) {
        .FunctionCall = {
            .id = NodeFunctionCall,
            .trace = stretch(token.trace, expect(parser->tokenizer, ')').trace),
            .function = new_node((Node) {
                .External = {
                    .id = NodeExternal,
                    .data = String("sizeof"),
                }
            }),
            .arguments = arguments,
            .type = (void*) find_on_stack(parser->stack, (Trace) { String("usize") }),
        }
    });
}

Node* keyword_const(const Token token, Parser* parser) {
    // TODO: (const) fix trace
    (void) token;

    Type* type = (void*) righthand_expression(lefthand_expression(parser), parser, 13);

    if(!(type->flags & fType)) {
        push(parser->tokenizer->messages, REPORT_ERR(type->trace,
                 String("expected a type after '\33[35mconst\33[0m'")));
        type = type->type;
    }

    type->flags |= fConst;
    return (void*) type;
}

Node* keyword_extern(const Token token, Parser* parser) {
    Type* type = NULL;
    unsigned long flags = 0;

    if(try(parser->tokenizer, '<', 0)) {
        // TODO: maybe move to a flag in `Parser`
        global_righthand_collecting_type_arguments = true;
        type = (void*) expression(parser);
        global_righthand_collecting_type_arguments = false;

        if(!(type->flags & fType)) {
            // TODO: error when (in extern<TYPE>) TYPE is not a type
            type = type->type;
        }

        expect(parser->tokenizer, '>');
    } else {
        flags |= fType;
        if(streq(parser->tokenizer->current.trace.source, String("int"))) {
            flags |= fNumeric;
            next(parser->tokenizer);
        }
    }

    Token external_token;
    if(!try(parser->tokenizer, TokenIdentifier, &external_token)) {
        external_token = expect(parser->tokenizer, TokenString);
    }

    const Trace trace = stretch(token.trace, external_token.trace);
    String data = external_token.trace.source;
    if(external_token.type == TokenString) {
        data.data++;
        data.size -= 2;
    }

    Node* external = new_node((Node) {
        .External = {
            .id = NodeExternal,
            .flags = fConstExpr | flags,
            .trace = trace,
            .type = type,
            .data = data,
        }
    });

    if(!external->type) {
        external->type = (void*) external;
    }

    return external;
}

Node* keyword_self(const Token token, Parser* parser) {
    StructType* structure = (void*) parser->stack.data[parser->stack.size - 2]->parent;
    if(structure->id != NodeStructType) return NULL;

    VariableDeclaration* const argument_declaration = (void*) new_node((Node) {
        .VariableDeclaration = {
            .id = NodeVariableDeclaration,
            .trace = token.trace,
            .type = make_type_standalone((void*) structure),
            .identifier = { .base = String("self") },
        }
    });
    argument_declaration->identifier.parent_declaration = (void*) argument_declaration;

    put(&last(parser->stack)->variables, token.trace.source, (void*) argument_declaration);

    Wrapper* variable = variable_of((void*) argument_declaration, token.trace, fIgnoreStatement);
    variable->Variable.is_self_literal = true;
    return (void*) variable;
}
