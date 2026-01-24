#ifndef COMPILER_IDENTIFIER_H
#define COMPILER_IDENTIFIER_H

#include "../../compiler.h"

extern DeclarationHashMap global_declaration_space;

void populate_global_c_keywords();

void build_simple_identifier(String identifier, String* application);

void build_full_identifier(Identifier identifier, String* application);

bool resolve_identifier(Identifier identifier, String* identifier_builder);

#endif
