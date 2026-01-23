#include "struct_declaration.h"

#include "identifier.h"

void comp_StructDeclaration(void* void_self, String* line, Compiler* compiler) {
    StructDeclaration* const self = void_self;
    StructType* const struct_type = (void*) self->type;

    if(self->identifier.is_external || self->compilation_state == CompilationSkip) return;

    String typedef_line = strf(0, "struct ");
    compile_identifier(self->identifier, &typedef_line);

    if(self->compilation_state == CompilationIntermediate) {
        push(&compiler->sections.data[0].lines, typedef_line);
        self->compilation_state = CompilationUnused;
        return;
    }

    self->compilation_state = CompilationIntermediate;

    strf(&typedef_line, " { ");
    for(size_t i = 0; i < struct_type->fields.size; i++) {
        compile(struct_type->fields.data[i].type, &typedef_line, compiler);
        strf(&typedef_line, " ");
        compile_identifier_base(struct_type->fields.data[i].identifier, &typedef_line);
        strf(&typedef_line, "; ");
    }
    strf(&typedef_line, "};");

    push(&compiler->sections.data[0].lines, typedef_line);
    compile(struct_type->static_body, line, compiler);

    if(!struct_type->reference_structures) return;
    for(size_t i = 0; i < sizeof(*struct_type->reference_structures) / sizeof(**struct_type->reference_structures); i++) {
        for(size_t j = 0; j < (*struct_type->reference_structures)[i].size; j++) {
            compile(&(*struct_type->reference_structures)[i].data[j].v, line, compiler);
        }
    }

    self->compilation_state = CompilationSkip;
}
