#include "types.h"

#include "../righthand/declaration/identifier.h"

void comp_PointerType(void* void_self, String* line, Compiler* compiler) {
    PointerType* self = void_self;

    compile(self->base, line, compiler);
    strf(line, "*");
}

void comp_FunctionType(void* void_self, String* line, Compiler* compiler) {
    FunctionType* self = void_self;

    String identifier = strf(0, "__Function__");
    resolve_identifier(self->declaration->identifier, &identifier);

    bool* type_definition_state = get(self->type_definitions, identifier);

    if(!type_definition_state) {
        put(&self->type_definitions, identifier, false);

        String typedef_line = strf(0, "typedef ");
        compile(self->signature.data[0], &typedef_line, compiler);
        strf(&typedef_line, " (*%.*s)(", FMT(identifier));

        for(size_t i = 1; i < self->signature.size; i++) {
            if(i > 1) strf(&typedef_line, ", ");
            compile(self->signature.data[i], &typedef_line, compiler);
        }

        strf(&typedef_line, ");");
        push(&compiler->sections.data[1].lines, typedef_line);

        *get(self->type_definitions, identifier) = true;
    } else if(!*type_definition_state) {
        strf(line, "/* circular */ void*");
        return;
    }

    strf(line, "%.*s", FMT(identifier));
}

void comp_GenericReference(void* void_self, String* line, Compiler* compiler) {
    GenericReference* const self = void_self;
    compile(last(self->generics_declaration->generics.type_arguments_stack).data[self->index], line, compiler);
}

void comp_StructType(void* void_self, String* line, Compiler* compiler) {
    (void) compiler;
    StructType* self = void_self;

    strf(line, "struct ");
    resolve_identifier(self->parent->identifier, line);
}

