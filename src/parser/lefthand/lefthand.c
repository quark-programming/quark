#include "lefthand.h"

#include "keywords.h"
#include "reference.h"
#include "../literal/array.h"
#include "../literal/string.h"
#include "../righthand/righthand.h"
#include "../type/types.h"
#include "../statement/scope.h"
#include "../righthand/declaration/identifier.h"
#include "../statement/structure.h"
#include "../literal/number.h"
#include "../keywords.h"
#include "parser/type/clash_types.h"

Type* boolean_type() {
    static Type boolean_type = {
        .External = {
            .id = NodeExternal,
            .type = &boolean_type,
            .flags = fType | fNumeric,
            .data = str("bool"),
        },
    };
    return &boolean_type;
}

Node* lefthand_expression(Parser* parser) {
    Token token = next(parser->tokenizer);

    switch(token.type) {
        case TokenNumber: return (void*) numeric_literal_from_token(token);

        case TokenIdentifier: {
            if(!token.identifier.searched_keyword) {
                Keyword* const keyword_info = get(global_keyword_table, token.trace.source);
                if(keyword_info) {
                    token.identifier.is_keyword = true;
                    token.identifier.keyword = *keyword_info;
                }
            }

            if(token.identifier.is_keyword && !(token.identifier.keyword.specific_action & KeywordActionStatement)) {
                return token.identifier.keyword.consumer(token, parser);
            }

            const IdentifierInfo info = new_identifier(token, parser, 0);

            if(!info.value) {
                return (void*) new_type((Type) {
                    .Missing = {
                        .id = NodeMissing,
                        .trace = token.trace,
                    }
                });
            }

            if(info.value->flags & fType && try(parser->tokenizer, '{', 0)) {
                return parse_struct_literal((void*) info.value, parser);
            }

            return (void*) info.value;
        }

        case '(': {
            // TODO: wrap in surround and remove parenthesis in compiler to remove redundant parenthesis
            Node* expr = expression(parser);
            expect(parser->tokenizer, ')');

            return new_node((Node) {
                .Wrapper = {
                    .id = WrapperSurround,
                    .trace = expr->trace,
                    .type = expr->type,
                    .flags = expr->flags,
                    .Surround = { expr, str("("), str(")") },
                },
            });
        }

        case TokenString: return string_literal(token, parser);
        case TokenCharacter: return character_literal(token, parser);

        case '&': {
            Node* expression = righthand_expression(lefthand_expression(parser), parser, 2);
            return reference(expression, stretch(token.trace, expression->trace));
        }

        case '*': {
            Node* expression = righthand_expression(lefthand_expression(parser), parser, 2);
            return dereference(expression, stretch(token.trace, expression->trace), parser->tokenizer->messages);
        }

        case '[': return parse_array_literal(token.trace, parser);

        case '!': {
            Node* expr = righthand_expression(lefthand_expression(parser), parser, 2);
            return new_node((Node) {
                .Wrapper = {
                    .id = WrapperSurround,
                    .type = boolean_type(),
                    .trace = expr->trace,
                    .Surround = { expr, str("!") },
                },
            });
        }

        case '-': {
            Node* expr = righthand_expression(lefthand_expression(parser), parser, 2);
            return new_node((Node) {
                .Wrapper = {
                    .id = WrapperSurround,
                    .type = expr->type,
                    .trace = expr->trace,
                    .Surround = { expr, str("-") },
                }
            });
        }

        default:
            push(parser->tokenizer->messages, MERROR(token.trace,
                     strf(0, "expected a \33[35mliteral\33[0m, but got '\33[35m%.*s\33[0m'",
                         fmtof(token.trace.source))));
            return (void*) numeric_literal_from_token(token);
    }
}
