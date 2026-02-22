#include "structure.h"
#include "../type/types.h"
#include "../righthand/righthand.h"
#include "../righthand/declaration/declaration.h"
#include "parser/type/stringify_type.h"

// TODO: add `Trace trace` argument for info.trace (or `IdentifierInfo info` argument)
Node* parse_struct_literal(Type* const wrapped_struct_type, Parser* parser) {
    const OpenedType opened = open_type(wrapped_struct_type, 0);
    StructType* const struct_type = (void*) opened.type;

    // TODO: error message if not struct
    if(struct_type->id != NodeStructType) {
        push(parser->tokenizer->messages,
             MERROR(wrapped_struct_type->trace, str("creating structure literal with a non-structure type")));

        close_type(opened.actions, 0);
        return (void*) wrapped_struct_type;
    }

    StructLiteral* struct_literal = (void*) new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = (void*) wrapped_struct_type,
        }
    });

    while(parser->tokenizer->current.type && parser->tokenizer->current.type != '}') {
        if(parser->tokenizer->current.type == TokenIdentifier) {
            const Token field_name = next(parser->tokenizer);

            if(try(parser->tokenizer, ':', NULL)) {
                push(&struct_literal->field_names, field_name.trace.source);
                push(&struct_literal->field_values, expression(parser));

                for(size_t i = 0; i < len(struct_type->fields); i++) {
                    if(streq(struct_type->fields[i].identifier, field_name.trace.source)) {
                        goto continue_;
                    }
                }

                String message = strf(0, "no field named '\33[35m%.*s\33[35m' on '\33[35m").as_owned;
                stringify_type((void*) struct_type, &message, 0);
                push(parser->tokenizer->messages, MERROR(field_name.trace, strf(&message, "\33[0m'")));

                continue_:
                    if(!try(parser->tokenizer, ',', 0)) break;
                continue;
            }

            parser->tokenizer->current = field_name;
        }

        push(&struct_literal->field_names, { 0 });
        push(&struct_literal->field_values, expression(parser));

        if(!try(parser->tokenizer, ',', 0)) break;
    }

    struct_literal->trace = stretch(wrapped_struct_type->trace, expect(parser->tokenizer, '}').trace);
    close_type(opened.actions, 0);
    return (void*) struct_literal;
}
