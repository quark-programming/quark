#include "number.h"
#include "../type/types.h"

NumericLiteral* numeric_literal_from_token(const Token token) {
    return (void*) new_node((Node) {
        .NumericLiteral = {
            .id = NodeNumericLiteral,
            .trace = token.trace,
            .type = new_type((Type) {
                .Wrapper = {
                    .id = WrapperAuto,
                    .trace = token.trace,
                    .flags = fConstExpr | fNumeric,
                }
            }),
            .flags = fConstExpr,
            .value = strtol(token.trace.source.data, 0, 0),
        }
    });
}