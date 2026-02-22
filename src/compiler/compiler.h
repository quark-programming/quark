#ifndef COMPILER_H
#define COMPILER_H

#include "../parser/nodes/nodes.h"

typedef struct {
    Vec(String) lines;
    String indent;
} CompilerSection;

struct Compiler {
    Vec(CompilerSection) sections;
    size_t open_section;
    Vec(Message)* messages;
};

String new_line(Compiler* compiler);

void compile(void* void_node, String* line, Compiler* compiler);

#endif