#ifndef HELPERS_H
#define HELPERS_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

char* fs_readfile(char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;

    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);

    char* data = malloc(filesize + 1);
    data[fread(data, 1, filesize, file)] = 0;

    fclose(file);
    return data;
}

size_t calculate_string_length(const char* string_literal, size_t length) {
    size_t result = 0;

    for (size_t i = 1; i < length - 1; i++) {
        if (string_literal[i] == '\\' && i + 1 < length - 1) {
            char next = string_literal[i + 1];
            switch (next) {
            case 'n': case 't': case 'r': case '\\': case '"': case '\'':
            case '0':
                i++;
                break;
            case 'x':
                if (i + 3 < length - 1) {
                    i += 3;
                }
                break;
            default: assert(0 && "Invalid escape sequence");
            }
        } // TODO: add more escape sequences
        result++;
    }

    return result;
}

#endif
