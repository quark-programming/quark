#include "fs.h"

char* fs_readfile(const char* const filename) {
    FILE* file = fopen(filename, "r");
    if(!file) return 0;

    fseek(file, 0, SEEK_END);
    const size_t filesize = ftell(file);
    rewind(file);

    char* data = malloc(filesize + 1);
    data[fread(data, 1, filesize, file)] = 0;

    fclose(file);
    return data;
}

Path fs_path(char* path, Path* const builder) {
    Path path_builder = builder ? *builder : NULL;

    for(char* next_path; (next_path = strchr(path, '/')); path = next_path + 1) {
        const str slice = { next_path - path, path };
        if(len(path_builder) && streq(slice, str("..")))
            pop(&path_builder);
        else
            push(&path_builder, slice);
    }

    if(builder) *builder = path_builder;
    return path_builder;
}

String fs_path_join(Path path) {
    String builder = len(path) ? NULL : strf(0, ".").as_owned;

    for(u32 i = 0; i < len(path); i++) {
        strf(&builder, i ? "/%.*s" : "%.*s", fmtof(path[i]));
    }

    return builder;
}