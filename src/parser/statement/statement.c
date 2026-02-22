#include "statement.h"

#include "scope.h"
#include "../keywords.h"
#include "../righthand/righthand.h"

Node* statement(Parser* parser) {
    if(parser->tokenizer->current.type == TokenIdentifier) {
        Token* token = &parser->tokenizer->current;

        Keyword* const keyword_info = get(global_keyword_table, token->trace.source);
        if(keyword_info) {
            token->identifier.is_keyword = true;
            token->identifier.searched_keyword = true;
            token->identifier.keyword = *keyword_info;

            if(keyword_info->specific_action & KeywordActionStatement) {
                return keyword_info->consumer(next(parser->tokenizer), parser);
            }
        }
    }

    if(try(parser->tokenizer, '{', NULL)) {
        Scope* block_scope = new_scope(last(parser->stack)->declaration);

        push(&parser->stack, block_scope);
        block_scope->children = collect_until(parser, &statement, 0, '}');
        pop(&parser->stack);

        block_scope->wrap_with_brackets = true;
        return (void*) block_scope;
    }

    Node* expr = expression(parser);
    if(!(expr->flags & fStatementTerminated)) expect(parser->tokenizer, ';');

    if(expr->flags & fIgnoreStatement) {
        return new_node((Node) { NodeNone, .type = (void*) expr });
    }

    return new_node((Node) {
        .StatementWrapper = {
            .id = NodeStatementWrapper,
            .expression = expr,
        }
    });
}

Vec(Node*) collect_until(Parser* parser, Node* (*supplier)(Parser*), const char separator, const char terminator) {
    Vec(Node*) collection = NULL;

    while(parser->tokenizer->current.type && parser->tokenizer->current.type != terminator) {
        push(&collection, supplier(parser));
        if(separator && !try(parser->tokenizer, separator, 0)) break;
    }
    if(terminator) expect(parser->tokenizer, terminator);

    return collection;
}
