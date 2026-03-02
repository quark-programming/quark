#ifndef COMPILER_LITERALS_H
#define COMPILER_LITERALS_H

#include "../compiler.h"

void comp_NumericLiteral(void* void_self, String* line, Compiler* compiler);

void comp_Missing(void* void_self, String* line, Compiler* compiler);

void comp_External(void* void_self, String* line, Compiler* compiler);

void comp_StructLiteral(void* void_self, String* line, Compiler* compiler);

void comp_Cast(void* void_self, String* line, Compiler* compiler);

void comp_Reference(void* void_self, String* line, Compiler* compiler);

#endif