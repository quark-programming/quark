#include "scope.h"

void comp_Scope(void* void_self, String* line, Compiler* compiler) {
    Scope* const self = void_self;

    if(self->wrap_with_brackets) {
        String open_bracket = new_line(compiler);
        push(&compiler->sections[compiler->open_section].lines, strf(&open_bracket, "{").as_owned);
    }

    CompilerSection* const section = compiler->sections + compiler->open_section;
    strf(&section->indent, "    ");

    for(size_t i = 0; i < len(self->children); i++) {
        compile(self->children[i], line, compiler);
    }

    len(compiler->sections[compiler->open_section].indent) -= 4;

    if(self->wrap_with_brackets) {
        String close_bracket = new_line(compiler);
        push(&compiler->sections[compiler->open_section].lines, strf(&close_bracket, "}").as_owned);
    }

    if(self->result_value && !(self->result_value->flags & fType)) {
        compile(self->result_value, line, compiler);
    }
}
