#ifndef VECTOR_STRING_H
#define VECTOR_STRING_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "vector.h"
#include "helpers.h"

typedef Vector(char) String;

typedef Vector(String) StringVector;

#define String(lit) ((String) { sizeof(lit) - 1, 0, lit })

#ifndef STRF_BUFFER_SIZE
#define STRF_BUFFER_SIZE 65536
#endif

String strf(String* self, notnull const char* fmt, ...);

bool streq(String a, String b);

uint32_t fnv1a_u32_hash(String string);

#define PRINT(string) (int) (string).size, (string).data

#endif // VECTOR_STRING_H
