#ifndef FS
#define FS

#include <stdio.h>
#include <stdlib.h>

char* fs_readfile(char* filename) {
	FILE* file = fopen(filename, "r");
	if(!file) return 0;

	fseek(file, 0, SEEK_END);
	size_t filesize = ftell(file);
	rewind(file);

	char* data = malloc(filesize + 1);
	data[fread(data, 1, filesize, file)] = 0;

	fclose(file);
	return data;
}

#endif
