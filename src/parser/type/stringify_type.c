#include "stringify_type.h"

void stringify_generics(String* string, const TypeVector generics, const unsigned flags) {
    if (!generics.size) return;

    if (!(flags & StringifyAlphaNumeric)) {
        strf(string, "<");
    }

    for (size_t i = 0; i < generics.size; i++) {
        if (flags & StringifyAlphaNumeric) {
            strf(string, "__");
        } else if (i) {
            strf(string, ", ");
        }

        stringify_type(generics.data[i], string, flags);
    }

    if (!(flags & StringifyAlphaNumeric)) {
        strf(string, ">");
    }
}
