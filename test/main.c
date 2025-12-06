#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
struct Array__number { int* data; size_t size; };
struct Array__char { char* data; size_t size; };

int main();


int main() {
    struct Array__number numbers;
    struct Array__char string;
    (string = (struct Array__char) { .data = "Hello World", .size = 11 });
}
