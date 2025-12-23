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

Node* lefthand_expression(Parser* parser) {
    const Token token = next(parser->tokenizer);

    switch(token.type) {
        case TokenNumber: return (void*) numeric_literal_from_token(token);

        case TokenIdentifier: {
            bool use_existing_info = false;
            IdentifierInfo info = {};

            if(token.identifier.is_keyword) {
                switch(token.identifier.keyword.specific_action) {
                    case KeywordActionSelf:
                        info = new_identifier(token, parser, 0);
                        use_existing_info = true;
                        // TODO: check if parsing arguments (add `ParsingArguments` flag to parser)
                        if(info.value || parser->stack.size < 2) break;

                    default:
                        return token.identifier.keyword.consumer(token, parser);
                }
            }

            // TODO: unit tests

            if(!use_existing_info) {
                info = new_identifier(token, parser, 0);
            }

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
            Node* expr = expression(parser);
            expect(parser->tokenizer, ')');
            return expr;
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

        default:
            push(parser->tokenizer->messages, REPORT_ERR(token.trace,
                     strf(0, "expected a \33[35mliteral\33[0m, but got '\33[35m%.*s\33[0m'",
                         (int) token.trace.source.size, token.trace.source.data)));
            return (void*) numeric_literal_from_token(token);
    }
}
