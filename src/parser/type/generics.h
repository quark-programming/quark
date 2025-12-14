#ifndef GENERICS_H
#define GENERICS_H

#include "../nodes/nodes.h"
#include "../parser.h"

Type* wrap_applied_generics(Type* type, TypeVector generics, Declaration* declaration);

void assign_generics(struct Wrapper* variable, Parser* parser);

typedef Vector(Declaration**) DeclarationSetterVector;

typedef struct GenericsCollection {
    TypeVector base_type_arguments;
    Scope* generic_declarations_scope;
    DeclarationSetterVector declaration_setters;
} GenericsCollection;

GenericsCollection collect_generics(Parser* parser);

void assign_generics_to_declaration(Declaration* declaration, GenericsCollection collection);

void close_generics_declaration(Declaration* declaration);


#endif