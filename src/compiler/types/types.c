#include "types.h"

#include "../righthand/declaration/identifier.h"

void comp_PointerType(void* void_self, String* line, Compiler* compiler) {
    PointerType* self = void_self;

    compile(self->base, line, compiler);
    strf(line, "*");
}

void comp_FunctionType(void* void_self, String* line, Compiler* compiler) {
    FunctionType* self = void_self;

    String identifier = strf(0, "__Function__").as_owned;
    resolve_identifier(self->declaration->identifier, &identifier);

    bool* type_definition_state = get(self->type_definitions, as_str(identifier));

    if(!type_definition_state) {
        put(&self->type_definitions, as_str(identifier), false);

        String typedef_line = strf(0, "typedef ").as_owned;
        compile(self->signature[0], &typedef_line, compiler);
        strf(&typedef_line, " (*%.*s)(", fmtof(identifier));

        for(size_t i = 1; i < len(self->signature); i++) {
            strf(&typedef_line, i > 1 ? ", " : "");
            compile(self->signature[i], &typedef_line, compiler);
        }

        strf(&typedef_line, ");");
        push(&compiler->sections[1].lines, typedef_line);

        *get(self->type_definitions, as_str(identifier)) = true;
    } else if(!*type_definition_state) {
        strf(line, "/* circular */ void*");
        return;
    }

    strf(line, "%.*s", fmtof(identifier));
}

void comp_GenericReference(void* void_self, String* line, Compiler* compiler) {
    GenericReference* const self = void_self;
    compile(last(self->generics_declaration->generics.type_arguments_stack)[self->index], line, compiler);
}

void comp_StructType(void* void_self, String* line, Compiler* compiler) {
    (void) compiler;
    StructType* self = void_self;

    strf(line, "struct ");
    resolve_identifier(self->parent->identifier, line);
}
