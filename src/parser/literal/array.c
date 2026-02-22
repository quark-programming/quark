#include "array.h"

#include "clargs.h"
#include "wrapper.h"
#include "../statement/statement.h"
#include "../righthand/righthand.h"
#include "../type/clash_types.h"
#include "../type/types.h"
#include "parser/statement/scope.h"

Declaration* fetch_slice_declaration(Parser* parser) {
    static Declaration* declaration = NULL;

    if(!declaration) {
        declaration = find_on_stack_unwrapped(parser->stack, str("Slice"));

        if(!declaration) {
            panicf("[fatal] failed to find declaration for Slice<T>");
        }
    }

    return declaration;
}

Node* parse_array_literal(const Trace trace_start, Parser* parser) {
    Vec(Node*) const field_values = collect_until(parser, &expression, ',', ']');

    if(field_values && field_values[0]->flags & fType) {
        Wrapper* slice = variable_of(fetch_slice_declaration(parser), field_values[0]->trace, 0);

        Vec(Type*) generics = { 0 };
        push(&generics, (void*) field_values[0]);
        slice->action = (Action) { ActionApplyGenerics, generics, slice->Variable.declaration };

        return (void*) slice;
    }

    Vec(str) field_names = NULL;
    resv(&field_names, len(field_values));
    memset(field_names, 0, len(field_values) * sizeof(str));

    Type* array_type = new_type((Type) {
        .Wrapper = {
            .id = WrapperAuto,
            .trace = trace_start,
        },
    });

    for(u32 i = 0; i < len(field_values); i++) {
        clash_types(array_type, field_values[i]->type, field_values[i]->trace, parser->tokenizer->messages, 0);
    }

    static Vec(str) empty_field_names = NULL;
    if(!empty_field_names) {
        push(&empty_field_names, { 0 }, { 0 });
    }

    Node* slice = eval("array", "Slice {}", parser);
    slice->StructLiteral.type->Wrapper.action.generics[0] = array_type;
    slice->StructLiteral.field_names = empty_field_names;

    Node* data_literal = new_node((Node) {
        .StructLiteral = {
            .id = NodeStructLiteral,
            .type = new_type((Type) {
                .Wrapper = {
                    .id = WrapperSurround,
                    .Surround = { (void*) array_type, {}, str("[]") },
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
                .value = (i64) len(field_values),
            },
        }
    );
    push(&slice->StructLiteral.field_values, array_size);

    return slice;
}
