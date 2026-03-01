#ifndef GENERICS_H
#define GENERICS_H

#include "../nodes/nodes.h"
#include "../parser.h"

Node* assign_action(Node* node, Action action, bool important, bool owned_variable);

void apply_type_arguments(Wrapper* variable, Parser* parser);

typedef struct GenericsCollection {
    Vec(Type*) base_type_arguments;
    Scope* generic_declarations_scope;
} GenericsCollection;

GenericsCollection collect_generics(Parser* parser);

void assign_generics_to_declaration(Declaration* declaration, GenericsCollection collection);

void close_generics_declaration(Declaration* declaration);

#endif