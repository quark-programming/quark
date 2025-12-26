#include "clargs.h"
#include "compiler/block.c"
#include "helpers.h"

typedef Vector(char*) Str_Vector;

int in_compiler_step = 0;

FunctionDeclaration* entry_declaration() {
    FunctionType* function_type =
        (void*)new_type((Type){
            .FunctionType = {
                .compiler = (void*)&comp_FunctionType,
            }
        });
    push(&function_type->signature, new_type((Type) { .External = {
             .compiler = (void*) &comp_External,
             // TODO: warning: ISO C forbids conversion of function pointer to object pointer type
             .data = str("int"),
             }}));

    FunctionDeclaration* declaration =
        (void*)new_node((Node){
            .FunctionDeclaration = {
                .compiler = (void*)&comp_FunctionDeclaration,
                // TODO: warning: ISO C forbids conversion of function pointer to object pointer type
                .type = (void*)function_type,
                .identifier = (void*)new_node((Node){
                    .Identifier = {
                        .compiler = (void*)&comp_Identifier,
                        // TODO: warning: ISO C forbids conversion of function pointer to object pointer type
                        .base = str("main"),
                    }
                }),
                .body = new_scope(0),
            }
        });
    function_type->declaration = declaration;

    return declaration;
}

char* library_path = ".";
char* helps =
    " -l: library path    '-l /path/to/dir'\n"
    " -o: output c        '-o hello.c'\n"
    " hint: ./qc main.qk -o main.c -l ../dir \n"
    "";

int main(int argc, char** argv) {
    char* name = clname(argc, argv);
    (void)name; // TODO: maybe introduce UNUSED macro

    Str_Vector input_files = {0};
    char* output_file = "out.c";

    int flag;
    while ((flag = clflag()))
        switch (flag)
        {
        case 'h': printf("%s\n", helps);
            return 0;
        case -1: push(&input_files, clarg());
            break;
        case 'o': output_file = clarg();
            break;
        case 'l': library_path = clarg();
            break;
        default: panicf("unknown flag '-%c'\n", flag);
        }

    if (input_files.size == 0) {
        printf("%s\n", helps); return 0;
    }

    char* input_content = fs_readfile(input_files.data[0]);
    if (!input_content)
    {
        perror("read");
        panicf("unable to read file '%s'\n",
               input_files.data[0]);
    }

    Message_Vector messages = {0};
    Tokenizer tokenizer = new_tokenizer(input_files.data[0], input_content, &messages);
    Parser parser = {&tokenizer}; // TODO: also initialize stack (see warnings
    Compiler compiler = {.messages = &messages};

    push(&compiler.sections, (CompilerSection) { 0 });
    push(&compiler.sections, (CompilerSection) { 0 });
    push(&compiler.sections.data[0].lines, str("#include <stdint.h>"));
    push(&compiler.sections.data[0].lines, str("#include <stdio.h>"));
    push(&compiler.sections.data[0].lines, str("#include <string.h>"));
    push(&compiler.sections.data[0].lines, str("#include <stdlib.h>"));
    push(&compiler.sections.data[0].lines, str("#include <stdbool.h>"));

    push(&parser.stack, new_scope(0));

    FunctionDeclaration* entry = entry_declaration();
    push(&parser.stack, entry->body);

    // TODO: like really? just inserting import lib::std;
    push(&entry->body->children, eval_w("lib::std", "import lib::std;", &parser, &statement));
    NodeList body = collect_until(&parser, &statement, 0, 0);
    resv(&entry->body->children, body.size);
    for (size_t i = 0; i < body.size; i++)
    {
        push(&entry->body->children, body.data[i]);
    }

    in_compiler_step = 1;
    str temp_line = {0};
    puts("COMPILATION START");
    entry->compiler((void*)entry, &temp_line, &compiler);
    puts("COMPILATION COMPLETE");

    int error = 0;
    for (size_t i = 0; i < messages.size; i++)
    {
        if (print_message(messages.data[i])) error = 1;
    }

    if (error) return 1;

    FILE* out = fopen(output_file, "w+");
    if (!out)
    {
        perror("open");
        panicf("unable to output file '%s' to write\n",
               output_file);
    }

    for (size_t i = 0; i < compiler.sections.size; i++)
    {
        if (i) fprintf(out, "\n");
        for (size_t j = 0; j < compiler.sections.data[i].lines.size;
             j++)
        {
            fprintf(out, "%.*s\n",
                    (int)compiler.sections.data[i].lines.data[j].size,
                    compiler.sections.data[i].lines.data[j].data);
        }
    }
}
