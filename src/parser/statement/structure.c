#include "structure.h"
#include "../type/types.h"
#include "../righthand/righthand.h"
#include "../righthand/declaration/declarations.h"

// TODO: add `Trace trace` argument for info.trace (or `IdentifierInfo info` argument)
Node* parse_struct_literal(Type* const wrapped_struct_type, Parser* parser) {
    const OpenedType opened = open_type(wrapped_struct_type, 0);
    StructType* const struct_type = (void*) opened.type;

    // TODO: error message if not struct
    if(struct_type->id == NodeStructType) {
        close_type(opened.actions, 0);
        return NULL;
    }

    StructLiteral* struct_literal = (void*) new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = (void*) wrapped_struct_type,
        }
    });

    while(parser->tokenizer->current.type && parser->tokenizer->current.type != '}') {
        Node* field_value = expression(parser);
        String field_name = { 0 };

        if(try(parser->tokenizer, ':', NULL)) {
            // TODO: make sure field name is a `Missing` or `External` node
            const Trace field_name_trace = field_value->trace;
            field_name = field_value->trace.source;
            unbox(field_value);
            field_value = expression(parser);

            bool found_field_name = false;
            for(size_t i = 0; i < struct_type->fields.size; i++) {
                if(streq(field_name, struct_type->fields.data[i].identifier)) {
                    found_field_name = true;
                    break;
                }
            }

            if(!found_field_name) {
                push(parser->tokenizer->messages, REPORT_ERR(field_name_trace,
                         strf(0, "no field named '\33[35m%.*s\33[0m' on struct '\33[35m%.*s\33[0m'",
                             (int) field_name_trace.source.size, field_name_trace.source.data,
                             (int) wrapped_struct_type->trace.source.size, wrapped_struct_type->trace.source.data)));
                push(parser->tokenizer->messages,
                     see_declaration((void*) struct_type->parent, wrapped_struct_type->trace));
            }
        }

        push(&struct_literal->field_names, field_name);
        push(&struct_literal->field_values, field_value);

        if(!try(parser->tokenizer, ',', 0)) {
            break;
        }
    }

    struct_literal->trace = stretch(wrapped_struct_type->trace, expect(parser->tokenizer, '}').trace);
    close_type(opened.actions, 0);
    return (void*) struct_literal;
}
