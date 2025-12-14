#ifndef NODE_GENERICS_H
#define NODE_GENERICS_H

#include <helpers.h>
#include <hashmap.h>

#include "../fields.h"

typedef struct Generics {
    HashMap(UsableVoid) unique_combinations;
    Vector(TypeVector) type_arguments_stack;
    TypeVector base_type_arguments;
    void (*monomorphic_compiler)(void*, String*, Compiler*);
} Generics;

typedef struct GenericReference {
    TYPE_FIELDS;
    Declaration* generics_declaration;
    size_t index;
} GenericReference;

#endif