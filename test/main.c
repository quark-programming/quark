#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct Slice__char {
    char* data;
    size_t size;
};

struct str {
    struct Slice__char slice;
};

struct _UnwrappedVector__char {
    char* data;
    size_t size;
    size_t capacity;
};

struct Slice__struct_str {
    struct str* data;
    size_t size;
};

struct Range {
    size_t start;
    size_t end;
};

struct String {
    struct _UnwrappedVector__char vector;
};

int main();
size_t str__len(struct str);
char* str__data(struct str);
struct String str__to_owned(struct str);
struct _UnwrappedVector__char _UnwrappedVector__char__new();
int _UnwrappedVector__char__reserve(struct _UnwrappedVector__char*, size_t);
int _UnwrappedVector__char__push(struct _UnwrappedVector__char*, char);
int _UnwrappedVector__char__push_many(struct _UnwrappedVector__char*, struct Slice__char);
int _UnwrappedVector__char__pop(struct _UnwrappedVector__char*);
struct Range Range__new(size_t, size_t);
size_t Range__len(struct Range);
void write_file(FILE*, struct str);
void print_file(FILE*, struct str);
void print(struct str);
void panic(struct str);
struct String String__new();
struct String String__from(struct str);
struct str String__to_str(struct String);
struct String* String__cat(struct String*, struct str);
char* String__to_cstr(struct String*);


int main() {
}


size_t str__len(struct str self) {
    return ((self.slice).size);
}


char* str__data(struct str self) {
    return ((self.slice).data);
}


struct String str__to_owned(struct str self) {
    struct String string;
    (string = String__new());
    struct Slice__struct_str error_messages;
    _UnwrappedVector__char__push_many((&(string.vector)), (self.slice));
    return string;
}


struct _UnwrappedVector__char _UnwrappedVector__char__new() {
    return (struct _UnwrappedVector__char){.data = malloc(sizeof(char)), .size = 0, .capacity = 1};
}


int _UnwrappedVector__char__reserve(struct _UnwrappedVector__char* self, size_t n) {
    if ((((self->size) + n) <= (self->capacity)))
        return 0;
    while (((self->capacity) < ((self->size) + n)))
    {
        if ((((self->capacity) * 2) < (self->capacity)))
            return 1;
        ((self->capacity) *= 2);
    }
    ((self->data) = realloc((self->data), (self->capacity)));
    if (((self->data) == 0))
        return 2;
    return 0;
}


int _UnwrappedVector__char__push(struct _UnwrappedVector__char* self, char item) {
    int const reserve_result = _UnwrappedVector__char__reserve(self, 1);
    if (reserve_result)
        return reserve_result;
    ((*((self->data) + ((self->size)++))) = item);
    return 0;
}


int _UnwrappedVector__char__push_many(struct _UnwrappedVector__char* self, struct Slice__char slice) {
    int const reserve_result = _UnwrappedVector__char__reserve(self, (slice.size));
    if (reserve_result)
        return reserve_result;
    memcpy(((self->data) + (self->size)), (slice.data), (slice.size));
    return 0;
}


int _UnwrappedVector__char__pop(struct _UnwrappedVector__char* self) {
    if ((((self->size)--) == 0))
        return 3;
    return 0;
}


struct Range Range__new(size_t start, size_t end) {
    return (struct Range){start, end};
}


size_t Range__len(struct Range self) {
    return ((self.end) - (self.start));
}


void write_file(FILE* file, struct str message) {
    fwrite(((message.slice).data), ((message.slice).size), sizeof(char), file);
}


void print_file(FILE* file, struct str message) {
    char const newline = '\n';
    fwrite(((message.slice).data), ((message.slice).size), sizeof(char), file);
    fwrite((&newline), 1, sizeof(char), file);
}


void print(struct str message) {
    print_file(stdout, message);
}


void panic(struct str message) {
    write_file(stderr, (struct str){"\x1b[31mpanicked: \x1b[0m", (sizeof("\x1b[31mpanicked: \x1b[0m") - 1)});
    print_file(stderr, message);
    exit(EXIT_FAILURE);
}


struct String String__new() {
    struct String self;
    (self = (struct String){_UnwrappedVector__char__new()});
    return self;
}


struct String String__from(struct str string) {
    struct _UnwrappedVector__char vector;
    (vector = _UnwrappedVector__char__new());
    _UnwrappedVector__char__push_many((&vector), (string.slice));
    return (struct String){vector};
}


struct str String__to_str(struct String self) {
    return (struct str){(struct Slice__char){((self.vector).data), ((self.vector).size)}};
}


struct String* String__cat(struct String* self, struct str string) {
    _UnwrappedVector__char__push_many((&(self->vector)), (string.slice));
    return self;
}


char* String__to_cstr(struct String* self) {
    _UnwrappedVector__char__push((&(self->vector)), '\0');
    return ((self->vector).data);
}