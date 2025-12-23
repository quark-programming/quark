#include "string.h"
#include "../righthand/righthand.h"

size_t calculate_string_length(const char* string_literal, const size_t length) {
    size_t result = 0;

    for(size_t i = 1; i < length - 1; i++) {
        // TODO: this can be optimized into a table
        if(string_literal[i] == '\\' && i + 1 < length - 1) {
            const char next = string_literal[i + 1];
            switch(next) {
                case 'n':
                case 't':
                case 'r':
                case '\\':
                case '"':
                case '\'':
                case '0':
                    i++;
                    break;

                case 'x':
                    if(i + 3 < length - 1) {
                        i += 3;
                    }
                    break;

                default: unreachable();
            }
        } // TODO: add more escape sequences
        result++;
    }

    return result;
}

Node* string_literal(const Token token, Parser* parser) {
    const String string_data = token.trace.source;
    const size_t actual_length = calculate_string_length(string_data.data, string_data.size);

    static Type* char_pointer_type = NULL;
    if(!char_pointer_type) char_pointer_type = (void*) eval(NULL, "char*", parser);

    static Type referenceable_size_t_type = {
        .External = {
            .id = NodeExternal,
            .data = String("size_t"),
        }
    };

    static Type* string_slice_type = NULL;
    if(!string_slice_type) string_slice_type = (void*) eval(NULL, "str", parser);

    static StringVector empty_field_names = { 0 };
    if(!empty_field_names.size) {
        push(&empty_field_names, (String) { 0 });
        push(&empty_field_names, (String) { 0 });
    }

    NodeVector field_values = { 0 };

    Node* data_field = new_node((Node) {
        .External = {
            .id = NodeExternal,
            .type = (void*) eval(NULL, "char*", parser),
            .data = token.trace.source,
        }
    });
    push(&field_values, data_field);

    Node* size_field = new_node((Node) {
        .NumericLiteral = {
            .id = NodeNumericLiteral,
            .type = &referenceable_size_t_type,
            .flags = fConstExpr,
            .value = (long) actual_length,
        }
    });
    push(&field_values, size_field);

    return new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .trace = token.trace,
            .type = string_slice_type,
            .field_values = field_values,
            .field_names = empty_field_names,
        }
    });
}

Node* character_literal(const Token token, Parser* parser) {
    static Type* char_type = NULL;
    if(!char_type) char_type = (void*) eval(NULL, "char", parser);

    return new_node((Node) {
        .External = {
            .id = NodeExternal,
            .trace = token.trace,
            .type = char_type,
            .flags = fConst,
            .data = token.trace.source,
        }
    });
}
