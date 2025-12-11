#ifndef HASHMAP_H
#define HASHMAP_H

#include <assert.h>

#include "vector.h"
#include "vector-string.h"

#ifndef MAP_SIZE
#define MAP_SIZE 16
#endif

#define HashMap(V) typeof(Vector(struct { String k; V v; })(*)[MAP_SIZE])

#define put(map, key, value) do { \
    if(!*(map)) { \
        *(map) = calloc(1, sizeof(**(map))); \
        assert(*(map) && "Out of Memory"); \
    } \
    push(&(**(map))[fnv1a_u32_hash(key) % MAP_SIZE], (typeof(***(map))) { key, value }); \
} while(0)

void* hashmap__get(notnull void** map, String key, size_t size);

#define get(map, key) ((typeof((***(map)).v)*) hashmap__get((void*)(map), key, sizeof(***(map))))

#endif