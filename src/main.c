#include <clargs.h>
#include <helpers.h>
#include <errno.h>
#include <tty.h>

#include "compiler/compiler.h"
#include "parser/statement/scope.h"
#include "parser/statement/statement.h"
#include "parser/type/types.h"
#include "parser/keywords.h"
#include "compiler/righthand/declaration/identifier.h"

#define QUARK_VERSION "0.5"
#define QUARK_STABILITY "dev"

FunctionDeclaration* entry_declaration() {
    FunctionType* function_type = (void*) new_type((Type) { NodeFunctionType });
    Type* const int_type = new_type((Type) { .External = { NodeExternal, .data = str("int") } });
    push(&function_type->signature, int_type);

    FunctionDeclaration* declaration = (void*) new_node((Node) {
        .FunctionDeclaration = {
            .id = NodeEntryFunctionDeclaration,
            .type = (void*) function_type,
            .identifier = { .base = str("main") },
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
        "   -h    <no arguments>       prints help/usage menu\n"
        "   -v    <no arguments>       prints the current version of Quark\n"
        "   -o    /path/to/output.c    specifies compiled output path\n"
        "   -l    /path/to/library/    specifies the parent directory of `lib::std`\n";

int main(int argc, char** argv) {
    char* const name = *argv;

    Vec(char*) input_files = NULL;
    char* output_file = "out.c";

    Vec(char*) include_paths = vec((char*) "stdint.h", "stdio.h", "string.h", "stdlib.h", "stdbool.h");

    int flag;
    while((flag = clflag(&argc, &argv))) {
        switch(flag) {
            case 'h':
                printf(help_message, name, name);
                return 0;
            case 'v':
                puts("Quark Compiler version " QUARK_VERSION " \33[90m" QUARK_STABILITY "\33[0m");
                return 0;
            case -1:
                push(&input_files, clarg(&argc, &argv));
                break;
            case 'o':
                output_file = clarg(&argc, &argv);
                break;
            case 'l':
                push(&global_library_paths, clarg(&argc, &argv));
                break;
            case 'i':
                push(&include_paths, clarg(&argc, &argv));
                break;
            default: panicf("unknown flag '-%c'\n hint: %s -h\n", flag, name);
        }
    }

    push(&global_library_paths, ".");

    if(len(input_files) == 0) {
        panicf("missing input files\n hint: %s -h\n", name);
    }

    char* input_content = fs_readfile(input_files[0]);
    if(!input_content) {
        panicf("unable to read file '%s': %s\n", input_files[0], strerror(errno));
    }

    // TODO: flag to specify initial size
    init_node_arena(2048);
    populate_keyword_table();
    populate_global_c_keywords();
    init_tty();

    Vec(Message) messages = { 0 };
    Tokenizer tokenizer = new_tokenizer(input_files[0], input_content, &messages);
    Parser parser = { &tokenizer };

    Compiler compiler = {
        .messages = &messages,
        .sections = vec((CompilerSection) {
            .lines = vec(strf(NULL, "// Quark Version %s", QUARK_VERSION).as_owned)
        }, { 0 })
    };

    for(size_t i = 0; i < len(include_paths); i++) {
        push(&compiler.sections[0].lines, strf(0, "#include \"%s\"", include_paths[i]).as_owned);
    }

    push(&parser.stack, new_scope(NULL));

    FunctionDeclaration* entry = entry_declaration();
    push(&parser.stack, entry->body);

    push(&entry->body->children, eval_w("lib::std", "import lib::std;", &parser, &statement));
    Vec(Node*) const body = collect_until(&parser, &statement, 0, 0);

    resv(&entry->body->children, len(body));
    for(size_t i = 0; i < len(body); i++) {
        push(&entry->body->children, body[i]);
    }

    global_in_compiler_step = true;
    global_compiler_context = &compiler;
    String temp_line = NULL;
    compile(entry, &temp_line, &compiler);

    bool printed_error = false;
    for(size_t i = 0; i < len(messages); i++) {
        if(print_message(messages[i])) printed_error = true;
    }

    if(printed_error) return 1;

    FILE* out = fopen(output_file, "w+");
    if(!out) {
        panicf("unable to output file '%s' to write: %s\n", output_file, strerror(errno));
    }

    for(size_t i = 0; i < len(compiler.sections); i++) {
        for(size_t j = 0; j < len(compiler.sections[i].lines); j++) {
            fprintf(out, "%.*s\n", fmtof(compiler.sections[i].lines[j]));
        }
        if(i) fprintf(out, "\n");
    }
}
