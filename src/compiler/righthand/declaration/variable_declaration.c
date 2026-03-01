#include "variable_declaration.h"

#include "identifier.h"
#include "../../../parser/type/types.h"

void comp_VariableDeclaration(void* void_self, String* line, Compiler* compiler) {
    VariableDeclaration* const self = void_self;

    if((self->generics.base_type_arguments && !len(self->generics.type_arguments_stacks[0]))
       || self->identifier.is_external || (self->const_value && self->const_value->flags & fType))
        return;

    String identifier = NULL;
    if(self->compilation_state != CompilationLocal && resolve_identifier(self->identifier, &identifier)) {
        free(vbase(identifier));
        return;
    }

    String decl_line = self->static_value ? NULL : new_line(compiler);
    line = &decl_line;

    compile(self->type, line, compiler);
    strf(line, self->type->flags & fConst ? " const " : " ");

    if(identifier) strf(line, "%.*s", fmtof(identifier));
    else build_full_identifier(self->identifier, line);

    if(self->static_value && self->static_value->id != NodeNone) {
        strf(line, " = ");
        compile(self->static_value, line, compiler);
    } else if(self->const_value) {
        strf(line, " = ");
        compile(self->const_value, line, compiler);
    } else if(self->type->flags & fConst) {
        push(compiler->messages, MERROR(self->trace,
                 str("expected declaration with '\33[35mconst\33[0m' type to have a value")));
    }

    strf(line, ";");
    push(&compiler->sections[!self->static_value * compiler->open_section].lines, decl_line);
}
