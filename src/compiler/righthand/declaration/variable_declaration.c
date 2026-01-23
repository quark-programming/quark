#include "variable_declaration.h"

#include "identifier.h"
#include "../../../parser/type/types.h"

static void compile_struct_declaration(StructType* self, String* line, Compiler* compiler) {
    String typedef_line = strf(0, "struct ");
    compile_identifier(self->parent->identifier, &typedef_line);

    strf(&typedef_line, " { ");
    for(size_t i = 0; i < self->fields.size; i++) {
        compile(self->fields.data[i].type, &typedef_line, compiler);
        strf(&typedef_line, " ");
        compile_identifier_base(self->fields.data[i].identifier, &typedef_line);
        strf(&typedef_line, "; ");
    }
    strf(&typedef_line, "};");

    push(&compiler->sections.data[0].lines, typedef_line);
    compile(self->static_body, line, compiler);

    if(!self->reference_structures) return;
    for(size_t i = 0; i < sizeof(*self->reference_structures) / sizeof(**self->reference_structures); i++) {
        for(size_t j = 0; j < (*self->reference_structures)[i].size; j++) {
            compile(&(*self->reference_structures)[i].data[j].v, line, compiler);
        }
    }
}

void comp_VariableDeclaration(void* void_self, String* line, Compiler* compiler) {
    VariableDeclaration* const self = void_self;

    if((self->generics.base_type_arguments.size && !self->generics.type_arguments_stack.size)
       || self->identifier.is_external || self->skip_compilation)
        return;

    if(self->const_value && self->const_value->flags & fType) {
        const OpenedType opened = open_type((void*) self->const_value, 0);
        compile_struct_declaration((void*) opened.type, line, compiler);
        close_type(opened.actions, 0);
        return;
    }

    String decl_line = new_line(compiler);
    line = &decl_line;

    compile(self->type, line, compiler);
    strf(line, self->type->flags & fConst ? " const " : " ");
    compile_identifier(self->identifier, line);

    if(self->const_value) {
        strf(line, " = ");
        compile(self->const_value, line, compiler);
    } else if(self->type->flags & fConst) {
        push(compiler->messages, REPORT_ERR(self->trace,
                 String("expected declaration with '\33[35mconst\33[0m' type to have a value")));
    }

    strf(line, ";");
    push(&compiler->sections.data[compiler->open_section].lines, decl_line);
}
