#ifndef FS_H
#define FS_H

#include <stdio.h>
#include "helpers.h"
#include <wrap.h>

typedef Vec(str) Path;

char* fs_readfile(const char* filename);

Path fs_path(char* path, Path* builder);

String fs_path_join(Path path);

#endif