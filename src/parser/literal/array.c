#include "array.h"
#include "../statement/statement.h"
#include "../righthand/righthand.h"
#include "../type/clash_types.h"
#include "../type/types.h"

Node* parse_array_literal(const Trace trace_start, Parser* parser) {
    const NodeVector field_values = collect_until(parser, &expression, ',', ']');

    if(field_values.data && field_values.data[0]->flags & fType) {
        // TODO: check if not Slice
        // TODO: (or) remove `eval`s
        Type* slice = (void*) eval(NULL, "Slice", parser);
        clash_types(slice->Wrapper.action.generics.data[0], (void*) field_values.data[0], field_values.data[0]->trace,
                    parser->tokenizer->messages, 0);
        return (void*) slice;
    }

    StringVector field_names = { 0 };
    resv(&field_names, field_values.size);
    memset(field_names.data, 0, field_values.size * sizeof(String));

    Type* array_type = new_type((Type) {
        .Wrapper = {
            .id = WrapperAuto,
            .trace = trace_start,
        },
    });

    for(size_t i = 0; i < field_values.size; i++) {
        clash_types(array_type, field_values.data[i]->type, field_values.data[i]->trace, parser->tokenizer->messages,
                    0);
    }

    static StringVector empty_field_names = { 0 };
    if(!empty_field_names.size) {
        resv(&empty_field_names, 2);
        memset(empty_field_names.data, 0, sizeof(String[2]));
    }

    Node* slice = eval("array", "Slice {}", parser);
    slice->StructLiteral.type->Wrapper.action.generics.data[0]->Wrapper.Auto.ref = (void*) array_type;
    slice->StructLiteral.field_names = empty_field_names;

    Node* data_literal = new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = new_type((Type) {
                .Wrapper = {
                    .id = WrapperSurround,
                    .Surround = { (void*) array_type, {}, String("[]"), true },
                },
            }),
            .field_names = field_names,
            .field_values = field_values,
        }
    });
    push(&slice->StructLiteral.field_values, data_literal);

    Node* array_size = new_node((Node) {
            .NumericLiteral = {
                .id = NodeNumericLiteral,
                .value = (int64_t) field_values.size,
            },
        }
    );
    push(&slice->StructLiteral.field_values, array_size);

    return slice;
}
