#ifndef VECTOR
#define VECTOR

#include <stdlib.h>

#define Vector(t) struct { size_t size, cap; t* data; }

#define resv(vec, n) do { \
	if((vec)->size + n > (vec)->cap) { \
		while(((vec)->cap = (vec)->cap * 2 ?: 1) < (vec)->size + n); \
		(vec)->data = realloc((vec)->data, (vec)->cap \
				* sizeof(*(vec)->data)); \
	} \
} while(0)

#define push(vec, item) do { \
	resv(vec, 1); \
	(vec)->data[(vec)->size++] = (item); \
} while(0)

#define last(vec) (vec).data[(vec).size - 1]

#endif
