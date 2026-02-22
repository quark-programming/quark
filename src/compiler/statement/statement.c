#include "statement.h"

void comp_StatementWrapper(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    StatementWrapper* const self = void_self;

    String expression_line = new_line(compiler);
    compile(self->expression, &expression_line, compiler);
    strf(&expression_line, ";");
    push(&compiler->sections[compiler->open_section].lines, expression_line);
}

void comp_ReturnStatement(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    ReturnStatement* const self = void_self;

    String statement_line = new_line(compiler);
    strf(&statement_line, "return ");
    if(self->value) compile(self->value, &statement_line, compiler);
    push(&compiler->sections[compiler->open_section].lines, strf(&statement_line, ";").as_owned);
}

void comp_ControlStatement(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    ControlStatement* const self = void_self;

    String block = new_line(compiler);
    strf(&block, "%.*s(", fmtof(self->keyword));

    for(size_t i = 0; i < len(self->conditions); i++) {
        strf(&block, i ? "; " : "");
        compile(self->conditions[i], &block, compiler);
    }

    resv(&block, 4);
    push(&compiler->sections[compiler->open_section].lines, strf(&block, ") ").as_owned);
    compile(self->body, &block, compiler);
}
