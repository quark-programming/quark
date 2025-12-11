#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <assert.h>

#define Vector(t) struct { size_t size, cap; t* data; }

#define resv(vec, n) do { \
    if((vec)->size + n > (vec)->cap) { \
        if((vec)->cap == 0) (vec)->cap = 1; \
        do (vec)->cap *= 2; \
        while((vec)->cap < (vec)->size + n); \
        (vec)->data = realloc((vec)->data, (vec)->cap * sizeof(*(vec)->data)); \
        assert((vec)->data && "Out of Memory"); \
    } \
} while(0)

#define push(vec, item) do { \
    resv(vec, 1); \
    (vec)->data[(vec)->size++] = (item); \
} while(0)

#define pop(vec) (vec)->data[--(vec)->size]

#define last(vec) (vec).data[(vec).size - 1]

#endif // VECTOR_H
