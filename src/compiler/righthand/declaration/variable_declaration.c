#include "variable_declaration.h"

#include "identifier.h"
#include "../../../parser/type/types.h"

void comp_VariableDeclaration(void* void_self, String* line, Compiler* compiler) {
    VariableDeclaration* const self = void_self;

    if((self->generics.base_type_arguments.size && !self->generics.type_arguments_stack.size)
       || self->identifier.is_external || self->compilation_state == CompilationSkip
       || (self->const_value && self->const_value->flags & fType))
        return;

    self->compilation_state = CompilationSkip;

    // if(self->const_value && self->const_value->flags & fType) {
    //     const OpenedType opened = open_type((void*) self->const_value, 0);
    //     compile_struct_declaration((void*) opened.type, line, compiler);
    //     close_type(opened.actions, 0);
    //     return;
    // }

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
