#include "function_declaration.h"

#include "identifier.h"

static void function_declaration_compiler_hoisted(FunctionDeclaration* const self, Compiler* compiler,
                                                  String identifier, const bool hoisted) {
    const size_t previous_section = compiler->open_section;
    const size_t section = compiler->open_section = compiler->sections.size;
    push(&compiler->sections, (CompilerSection) { 0 });

    String declaration_line = new_line(compiler);

    compile(self->type->FunctionType.signature.data[0], &declaration_line, compiler);
    strf(&declaration_line, " %.*s", FMT(identifier));

    strf(&declaration_line, "(");
    for(size_t i = 0; i < self->arguments.size; i++) {
        if(i) strf(&declaration_line, ", ");

        compile(self->arguments.data[i].type, &declaration_line, compiler);
        if(!hoisted) {
            strf(&declaration_line, " ");
            build_simple_identifier(self->arguments.data[i].identifier, &declaration_line);
        }
    }
    strf(&declaration_line, hoisted ? ");" : ") {");
    push(&compiler->sections.data[!hoisted * section].lines, declaration_line);

    if(hoisted) {
        compiler->open_section = previous_section;
        return;
    }

    for(size_t i = 0; i < self->variable_declarations.size; i++) {
        compile(self->variable_declarations.data[i], &declaration_line, compiler);
    }

    compile(self->body, &declaration_line, compiler);

    String terminator_line = new_line(compiler);
    push(&compiler->sections.data[section].lines, strf(&terminator_line, "}"));
    compiler->open_section = previous_section;
}

void comp_FunctionDeclaration(void* void_self, String* line, Compiler* compiler) {
    (void) line;
    FunctionDeclaration* self = void_self;

    if(self->identifier.is_external || (self->generics.base_type_arguments.size
                                        && !self->generics.type_arguments_stack.size))
        return;

    String identifier = { 0 };
    if(!resolve_identifier(self->identifier, &identifier)) {
        function_declaration_compiler_hoisted(self, compiler, identifier, true);
        function_declaration_compiler_hoisted(self, compiler, identifier, false);
    }

    free(identifier.data);
}
