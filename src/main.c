#include <clargs.h>
#include <helpers.h>
#include <errno.h>

#include "compiler/compiler.h"
#include "parser/statement/scope.h"
#include "parser/statement/statement.h"
#include "parser/type/types.h"
#include "parser/keywords.h"
#include "compiler/righthand/declaration/identifier.h"

#define QUARK_VERSION "0.4 (feature/modules)"
#define QUARK_STABILITY "dev"

FunctionDeclaration* entry_declaration() {
    FunctionType* function_type = (void*) new_type((Type) { NodeFunctionType });
    Type* const int_type = new_type((Type) { .External = { NodeExternal, .data = String("int") } });
    push(&function_type->signature, int_type);

    FunctionDeclaration* declaration = (void*) new_node((Node) {
        .FunctionDeclaration = {
            .id = NodeEntryFunctionDeclaration,
            .type = (void*) function_type,
            .identifier = { .base = String("main") },
            .body = new_scope(NULL),
        }
    });
    declaration->identifier.parent_declaration = (void*) declaration;
    function_type->declaration = declaration;
    declaration->body->parent = (void*) declaration;

    return declaration;
}

const char* help_message =
        " \33[1musage:\33[0m %s [input_files,] [flags,]\n"
        "        %s main.qk -o main.c\n"
        " \33[1mflags:\33[0m\n"
        "   -h    <no arguments>      prints help/usage menu\n"
        "   -v    <no arguments>      prints the current version of Quark\n"
        "   -o    /path/to/output.c   specifies compiled output path\n"
        "   -l    /path/to/library/   specifies the parent directory of `lib::std`\n";

int main(int argc, char** argv) {
    char* name = clname(argc, argv);

    CStringVector input_files = { 0 };
    char* output_file = "out.c";

    CStringVector include_paths = { 0 };

    int flag;
    while((flag = clflag())) {
        switch(flag) {
            case 'h':
                printf(help_message, name, name);
                return 0;
            case 'v':
                puts("Quark Compiler version " QUARK_VERSION " \33[90m" QUARK_STABILITY "\33[0m");
                return 0;
            case -1:
                push(&input_files, clarg());
                break;
            case 'o':
                output_file = clarg();
                break;
            case 'l':
                push(&global_library_paths, clarg());
                break;
            case 'i':
                push(&include_paths, clarg());
                break;
            default: panicf("unknown flag '-%c'\n hint: %s -h\n", flag, name);
        }
    }

    push(&global_library_paths, ".");

    push(&include_paths, "stdint.h");
    push(&include_paths, "stdio.h");
    push(&include_paths, "string.h");
    push(&include_paths, "stdlib.h");
    push(&include_paths, "stdbool.h");

    if(input_files.size == 0) {
        panicf("missing input files\n hint: %s -h\n", name);
    }

    char* input_content = fs_readfile(input_files.data[0]);
    if(!input_content) {
        panicf("unable to read file '%s': %s\n", input_files.data[0], strerror(errno));
    }

    // TODO: flag to specify initial size
    init_node_arena(2048);

    MessageVector messages = { 0 };
    Tokenizer tokenizer = new_tokenizer(input_files.data[0], input_content, &messages);
    Parser parser = { &tokenizer };
    Compiler compiler = { .messages = &messages };

    push(&compiler.sections, (CompilerSection) { 0 });
    push(&compiler.sections, (CompilerSection) { 0 });

    push(&compiler.sections.data[0].lines, strf(0, "// Quark Version %s", QUARK_VERSION));

    for(size_t i = 0; i < include_paths.size; i++) {
        push(&compiler.sections.data[0].lines, strf(0, "#include \"%s\"", include_paths.data[i]));
    }
    // push(&compiler.sections.data[0].lines, String("#include <stdint.h>"));
    // push(&compiler.sections.data[0].lines, String("#include <stdio.h>"));
    // push(&compiler.sections.data[0].lines, String("#include <string.h>"));
    // push(&compiler.sections.data[0].lines, String("#include <stdlib.h>"));
    // push(&compiler.sections.data[0].lines, String("#include <stdbool.h>"));

    push(&parser.stack, new_scope(NULL));

    FunctionDeclaration* entry = entry_declaration();
    push(&parser.stack, entry->body);

    populate_keyword_table();
    populate_global_c_keywords();

    push(&entry->body->children, eval_w("lib::std", "import lib::std;", &parser, &statement));
    const NodeVector body = collect_until(&parser, &statement, 0, 0);
    resv(&entry->body->children, body.size);
    for(size_t i = 0; i < body.size; i++) {
        push(&entry->body->children, body.data[i]);
    }

    global_in_compiler_step = true;
    global_compiler_context = &compiler;
    String temp_line = { 0 };
    compile(entry, &temp_line, &compiler);

    bool printed_error = false;
    for(size_t i = 0; i < messages.size; i++) {
        if(print_message(messages.data[i])) printed_error = true;
    }

    if(printed_error) return 1;

    FILE* out = fopen(output_file, "w+");
    if(!out) {
        panicf("unable to output file '%s' to write: %s\n", output_file, strerror(errno));
    }

    for(size_t i = 0; i < compiler.sections.size; i++) {
        for(size_t j = 0; j < compiler.sections.data[i].lines.size; j++) {
            fprintf(out, "%.*s\n", FMT(compiler.sections.data[i].lines.data[j]));
        }
        if(i) fprintf(out, "\n");
    }
}
