#include "scope.h"

void comp_Scope(void* void_self, String* line, Compiler* compiler) {
    Scope* const self = void_self;

    if(self->wrap_with_brackets) {
        String open_bracket = new_line(compiler);
        push(&compiler->sections.data[compiler->open_section].lines, strf(&open_bracket, "{"));
    }

    CompilerSection* const section = compiler->sections.data + compiler->open_section;
    strf(&section->indent, "    ");

    for(size_t i = 0; i < self->children.size; i++) {
        compile(self->children.data[i], line, compiler);
    }

    compiler->sections.data[compiler->open_section].indent.size -= 4;

    if(self->wrap_with_brackets) {
        String close_bracket = new_line(compiler);
        push(&compiler->sections.data[compiler->open_section].lines, strf(&close_bracket, "}"));
    }

    if(self->result_value) {
        compile(self->result_value, line, compiler);
    }
}
