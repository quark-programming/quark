#include "variable_declaration.h"

#include "identifier.h"
#include "../../../parser/type/types.h"

void comp_VariableDeclaration(void* void_self, String* line, Compiler* compiler) {
    VariableDeclaration* const self = void_self;

    if((self->generics.base_type_arguments && !len(self->generics.type_arguments_stacks[0]))
       || self->identifier.is_external || (self->const_value && self->const_value->flags & fType))
        return;

    String decl_line = new_line(compiler);
    line = &decl_line;

    compile(self->type, line, compiler);
    strf(line, self->type->flags & fConst ? " const " : " ");
    build_full_identifier(self->identifier, line);

    if(self->const_value) {
        strf(line, " = ");
        compile(self->const_value, line, compiler);
    } else if(self->type->flags & fConst) {
        push(compiler->messages, MERROR(self->trace,
                 str("expected declaration with '\33[35mconst\33[0m' type to have a value")));
    }

    strf(line, ";");
    push(&compiler->sections[compiler->open_section].lines, decl_line);
}
