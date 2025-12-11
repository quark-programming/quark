#ifndef GENERICS_H
#define GENERICS_H

#include <helpers.h>

#include "../nodes.h"

typedef struct Generics {
    HashMap(UsableVoid) unique_combinations;
    Vector(TypeVector) type_arguments_stack;
    TypeVector base_type_arguments;
} Generics;

#endif