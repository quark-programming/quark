#include "stringify_type.h"

#include "traverse_type.h"
#include "types.h"

void stringify_generics(String* string, Vec(Type*) const generics, const unsigned flags) {
    if(!len(generics)) return;

    if(!(flags & StringifyAlphaNumeric)) {
        strf(string, "<");
    }

    for(size_t i = 0; i < len(generics); i++) {
        if(flags & StringifyAlphaNumeric) {
            strf(string, "__");
        } else if(i) {
            strf(string, ", ");
        }

        stringify_type(generics[i], string, flags);
    }

    if(!(flags & StringifyAlphaNumeric)) {
        strf(string, ">");
    }
}

static int stringify_acceptor(Type* type, Type* follower, void* void_accumulator) {
    (void) follower;
    StringifyAccumulator* const accumulator = void_accumulator;

    switch(type->id) {
        case WrapperAuto:
            strf(accumulator->string, type->flags & fNumeric
                 ? accumulator->flags & StringifyAlphaNumeric
                 ? "number"
                 : "~number"
                 : "auto");
            return 0;

        case NodeExternal:
            strf(accumulator->string, "%.*s", fmtof(type->External.data));
            return 0;

        case NodePointerType:
            strf(accumulator->string, accumulator->flags & StringifyAlphaNumeric ? "ptrto_" : "&");
            return 0;

        case NodeStructType:
            strf(accumulator->string, accumulator->flags & StringifyAlphaNumeric ? "struct_%.*s" : "struct %.*s",
                 fmtof(type->StructType.module->declaration->identifier.base));

            if(type->StructType.module->declaration->generics.base_type_arguments) {
                stringify_generics(accumulator->string,
                                   last(type->StructType.module->declaration->generics.type_arguments_stack),
                                   accumulator->flags);
            }

            return 1;

        default:
            strf(accumulator->string, accumulator->flags & StringifyAlphaNumeric ? "UNKNOWN" : "~unknown");
            return 0;
    }
}


void stringify_type(Type* type, String* string, const unsigned flags) {
    traverse_type(type, NULL, &stringify_acceptor, &(StringifyAccumulator) { string, flags },
                  ActionKeepGlobalState | ActionNoChildCompilation);
}
