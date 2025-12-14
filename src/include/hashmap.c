#include "hashmap.h"

void* hashmap__get(void* const map, const String key, const size_t size) {
    if(!map) return NULL;

    const Vector(void)* const section = map + fnv1a_u32_hash(key) % MAP_SIZE * size;
    for(size_t i = 0; i < section->size; i++) {
        if(streq(*(String*)(section->data + i * size), key)) {
            return section->data + i * size + sizeof(String);
        }
    }

    return NULL;
}
