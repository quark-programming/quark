#include "function_declaration.h"

#include "identifier.h"

static void function_declaration_compiler_hoisted(FunctionDeclaration* const self, Compiler* compiler,
                                                  str identifier, const bool hoisted) {
    const size_t previous_section = compiler->open_section;
    const u32 section = compiler->open_section = len(compiler->sections);
    push(&compiler->sections, { 0 });

    String declaration_line = new_line(compiler);

    compile(self->type->FunctionType.signature[0], &declaration_line, compiler);
    strf(&declaration_line, " %.*s", fmtof(identifier));

    strf(&declaration_line, "(");
    for(size_t i = 0; i < len(self->arguments); i++) {
        if(i) strf(&declaration_line, ", ");

        compile(self->arguments[i].type, &declaration_line, compiler);
        if(!hoisted) {
            strf(&declaration_line, " ");
            build_simple_identifier(self->arguments[i].identifier, &declaration_line);
        }
    }
    strf(&declaration_line, hoisted ? ");" : ") {");
    push(&compiler->sections[!hoisted * section].lines, declaration_line);

    if(hoisted) {
        compiler->open_section = previous_section;
        return;
    }

    for(size_t i = 0; i < len(self->variable_declarations); i++) {
        compile(self->variable_declarations[i], &declaration_line, compiler);
    }

    compile(self->body, &declaration_line, compiler);

    String terminator_line = new_line(compiler);
    push(&compiler->sections[section].lines, strf(&terminator_line, "}").as_owned);
    compiler->open_section = previous_section;
}

void comp_FunctionDeclaration(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    FunctionDeclaration* self = void_self;

    if(self->identifier.is_external || (self->generics.base_type_arguments
                                        && !len(self->generics.type_arguments_stack)))
        return;

    String identifier = { 0 };
    if(!resolve_identifier(self->identifier, &identifier)) {
        function_declaration_compiler_hoisted(self, compiler, as_str(identifier), true);
        function_declaration_compiler_hoisted(self, compiler, as_str(identifier), false);
    }

    free(vbase(identifier));
}
