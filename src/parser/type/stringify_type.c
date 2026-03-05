#include "stringify_type.h"

#include "traverse_type.h"
#include "types.h"

void stringify_generics(String* string, Vec(Type*) const generics, const unsigned flags, const u8 generics_offset) {
    if(!len(generics)) return;

    if(!(flags & StringifyAlphaNumeric)) {
        strf(string, "<");
    }

    for(size_t i = 0; i < len(generics); i++) {
        if(!(flags & StringifyShorthand)) {
            if(flags & StringifyAlphaNumeric) {
                strf(string, "__");
            } else if(i) {
                strf(string, ", ");
            }
        }

        stringify_type(generics[i], string, flags, generics_offset);
    }

    if(!(flags & StringifyAlphaNumeric)) {
        strf(string, ">");
    }
}

static void stringify_required_traits(Vec(Declaration*) required_traits, String* string, const unsigned flags) {
    for(u32 i = 0; i < len(required_traits); i++) {
        strf(string, " + ");

        switch(required_traits[i]->id) {
            case NodeFunctionDeclaration:
                strf(string, "%.*s::%.*s",
                     fmtof(required_traits[i]->identifier.parent_scope->declaration->identifier.base),
                     fmtof(required_traits[i]->identifier.base));
                break;

            case NodeStructDeclaration:
                // strf(string, "%.*s", fmtof(required_traits[i]->identifier.base));
                stringify_type(required_traits[i]->type, string, flags, 0);
                break;

            default: ;
        }
    }
}

static int stringify_acceptor(Type* type, Type* follower, void* void_accumulator) {
    (void) follower;
    StringifyAccumulator* const accumulator = void_accumulator;

    switch(type->id) {
        case WrapperAuto:
            if(type->Wrapper.Auto.missing) {
                strf(accumulator->string, "(%.*s)", fmtof(type->trace.source));
                return 0;
            }

            if(type->Wrapper.Auto.test_against && !(accumulator->flags & StringifyAlphaNumeric)) {
                stringify_type(type->Wrapper.Auto.test_against, accumulator->string, accumulator->flags,
                               accumulator->generics_offset);

                stringify_required_traits(type->Wrapper.Auto.required_traits, accumulator->string, accumulator->flags);
                return 0;
            }

            if(type->Wrapper.Auto.priority < 0 && !(accumulator->flags & StringifyAlphaNumeric)) {
                if(accumulator->flags & StringifyShorthand)
                    strf(accumulator->string, "%c", type->trace.source.data[0]);
                else
                    strf(accumulator->string, "%.*s", fmtof(type->trace.source));
            } else {
                if(accumulator->flags & StringifyShorthand)
                    strf(accumulator->string, type->flags & fNumeric ? "i" : "a");
                else
                    strf(accumulator->string, type->flags & fNumeric ? "int" : "auto");
            }

            if(type->Wrapper.Auto.required_traits && !(accumulator->flags & StringifyAlphaNumeric)) {
                stringify_required_traits(type->Wrapper.Auto.required_traits, accumulator->string, accumulator->flags);
            }

            return 0;

        case NodeExternal:
            if(accumulator->flags & StringifyShorthand)
                strf(accumulator->string, "%c",
                 type->External.shorthand ? type->External.shorthand : type->External.trace.source.data[0]);
            else
                strf(accumulator->string, "%.*s", fmtof(type->External.trace.source));
            return 0;

        case NodePointerType:
            if(accumulator->flags & StringifyShorthand)
                strf(accumulator->string, "p");
            else
                strf(accumulator->string, accumulator->flags & StringifyAlphaNumeric ? "ptrto_" : "&");
            return 0;

        case NodeStructType:
            if(accumulator->flags & StringifyShorthand)
                strf(accumulator->string, "%c", type->StructType.module->declaration->identifier.base.data[0]);
            else
                strf(accumulator->string, accumulator->flags & StringifyAlphaNumeric ? "struct_%.*s" : "%.*s",
                 fmtof(type->StructType.module->declaration->identifier.base));

            if(type->StructType.module->declaration->generics.base_type_arguments) {
                const Generics generics = type->StructType.module->declaration->generics;
                const u8 normal_offset = len(generics.type_arguments_stacks[accumulator->generics_offset])
                                             ? accumulator->generics_offset
                                             : 2;
                stringify_generics(accumulator->string, last(generics.type_arguments_stacks[normal_offset]),
                                   accumulator->flags, accumulator->generics_offset);
            }

            return 1;

        default:
            if(accumulator->flags & StringifyShorthand)
                strf(accumulator->string, "U");
            else
                strf(accumulator->string, accumulator->flags & StringifyAlphaNumeric ? "UNKNOWN" : "~unknown");
            return 0;
    }
}


void stringify_type(Type* type, String* string, const unsigned flags, const u8 generics_offset) {
    traverse_type(type, NULL, &stringify_acceptor, &(StringifyAccumulator) { string, flags, generics_offset },
                  ActionKeepGlobalState | ActionNoChildCompilation, generics_offset);
}
