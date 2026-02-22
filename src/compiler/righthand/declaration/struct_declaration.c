#include "struct_declaration.h"

#include "identifier.h"

void comp_StructDeclaration(void* void_self, String* line, Compiler* compiler) {
    StructDeclaration* const self = void_self;
    StructType* const struct_type = (void*) self->type;

    if(self->identifier.is_external) return;

    String identifier = { 0 };
    if(resolve_identifier(self->identifier, &identifier)) {
        if(self->compilation_state == CompilationIntermediate) {
            push(&compiler->sections[0].lines, strf(0, "struct %.*s;", fmtof(identifier)).as_owned);
            self->compilation_state = CompilationUnused;
        }

        free(vbase(identifier));
        return;
    }

    String typedef_line = strf(0, "struct %.*s", fmtof(identifier)).as_owned;
    self->compilation_state = CompilationIntermediate;

    strf(&typedef_line, " { ");
    for(size_t i = 0; i < len(struct_type->fields); i++) {
        compile(struct_type->fields[i].type, &typedef_line, compiler);
        strf(&typedef_line, " ");
        build_simple_identifier(struct_type->fields[i].identifier, &typedef_line);
        strf(&typedef_line, "; ");
    }
    strf(&typedef_line, "};");

    push(&compiler->sections[0].lines, typedef_line);
    compile(struct_type->module->scope, line, compiler);

    if(!struct_type->traits) return;
    // TODO: create wrap::Map entry iterator or just iterator functions in general
    for(u32 i = 0; i < WRAPMAPSIZE; i++) {
        unsigned char* const entry_list = (void*)(*struct_type->traits)[i];
        for(u32 j = 0; j < len((*struct_type->traits)[i]); j++) {
            compile(entry_list + j * (sizeof(str) + sizeof(Scope*)) + sizeof(str), line, compiler);
        }
    }

    self->compilation_state = CompilationSkip;
}
