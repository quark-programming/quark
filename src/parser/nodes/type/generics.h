#ifndef NODE_GENERICS_H
#define NODE_GENERICS_H

#include <helpers.h>

#include "../fields.h"

typedef struct Generics {
    Set unique_combinations;
    Vec(Vec(Type*)) type_arguments_stacks[3];
    Vec(Type*) base_type_arguments;
} Generics;

typedef struct GenericReference {
    TYPE_FIELDS;
    Declaration* generics_declaration;
    size_t index;
} GenericReference;

#endif