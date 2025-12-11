#include "vector-string.h"

String strf(String* self, const char* fmt, ...) {
    static char buffer[STRF_BUFFER_SIZE];
    va_list va_args;
    va_start(va_args, fmt);

    self = self ? self : &(String) { 0 };

    size_t bytes = vsnprintf(buffer, sizeof(buffer), fmt, va_args);
    resv(self, bytes);
    memcpy(self->data + self->size, buffer, bytes);
    self->size += bytes;
    return *self;
}

bool streq(const String a, const String b) {
    return a.size == b.size && !memcmp(a.data, b.data, a.size);
}

uint32_t fnv1a_u32_hash(String string) {
    uint32_t hash = 2166136261u;
    while(string.size--) hash = (hash ^ *string.data++) * 16777619;
    return hash;
}
