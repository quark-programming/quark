#include "statement.h"

void comp_StatementWrapper(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    StatementWrapper* const self = void_self;

    String expression_line = new_line(compiler);
    compile(self->expression, &expression_line, compiler);
    strf(&expression_line, ";");
    push(&compiler->sections.data[compiler->open_section].lines, expression_line);
}

void comp_ReturnStatement(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    ReturnStatement* const self = void_self;

    String statement_line = new_line(compiler);
    strf(&statement_line, "return ");
    if (self->value) compile(self->value, &statement_line, compiler);
    push(&compiler->sections.data[compiler->open_section].lines, strf(&statement_line, ";"));
}

void comp_ControlStatement(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    ControlStatement* const self = void_self;

    String block = new_line(compiler);
    strf(&block, "%.*s(", FMT(self->keyword));

    for (size_t i = 0; i < self->conditions.size; i++) {
        if (i) strf(&block, "; ");
        compile(self->conditions.data[i], &block, compiler);
    }

    resv(&block, 4);
    push(&compiler->sections.data[compiler->open_section].lines, strf(&block, ") "));
    compile(self->body, &block, compiler);
}
