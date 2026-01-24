#include "literals.h"

void comp_NumericLiteral(void* void_self, String* line, Compiler* compiler) {
    (void) compiler;
    NumericLiteral* const self = void_self;

    strf(line, "%ld", self->value);
}

void comp_Missing(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    Missing* const self = void_self;

    push(compiler->messages,
         REPORT_ERR(self->trace, strf(0, "cannot find '\33[35m%.*s\33[0m' in scope", FMT(self->trace.source))));
}

void comp_External(void* void_self, String* line, Compiler* compiler) {
    (void) compiler;
    External* const self = void_self;

    strf(line, "%.*s", FMT(self->data));
}

void comp_StructLiteral(void* void_self, String* line, Compiler* compiler) {
    StructLiteral* const self = void_self;

    strf(line, "(");
    compile(self->type, line, compiler);
    strf(line, ") {");

    for(size_t i = 0; i < self->field_values.size; i++) {
        strf(line, i ? ", " : " ");
        if(self->field_names.data[i].size) {
            strf(line, ".%.*s = ", FMT(self->field_names.data[i]));
        }
        compile(self->field_values.data[i], line, compiler);
    }
    strf(line, " }");
}

void comp_Cast(void* void_self, String* line, Compiler* compiler) {
    Cast* const self = void_self;

    strf(line, "(");
    compile(self->type, line, compiler);
    strf(line, ") ");
    compile(self->value, line, compiler);
}
