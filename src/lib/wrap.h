// wrap.h - https://github.com/ephf/wrap.h

#ifndef WRAP_H
#define WRAP_H

#if !defined(WRAPREALLOC) || !defined(WRAPNOPANIC)
#include <stdlib.h>
#endif
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifndef WRAPREALLOC
#define WRAPREALLOCNOSIZE
#define WRAPREALLOC(ptr, size, new_size) realloc(ptr, new_size)
#endif

#ifndef WRAPDEF
#define WRAPDEF
#endif

#define __WA(a, ...) a
#define __WAB(a, b, ...) a, b
#define __WC(a, b, c, ...) c

#define wpanicf(...) ( \
		fprintf(stderr, "[\33[31mpanicked\33[0m] at %s(%d):\n\t", __func__, __LINE__), \
		fprintf(stderr, __VA_ARGS__), exit(EXIT_FAILURE))
#define whard_cast(Type, val) ((union { typeof(val) a; Type b; }){(val)}.b)

#ifndef WRAPNOSTRIP
#define panicf(...) wpanicf(__VA_ARGS__)
#define hard_cast(Type, val) whard_cast(Type, val)
#endif

typedef struct {
	uint32_t size, byte_capacity;
	unsigned char data[];
} __WVHead;

#define WVec(T) T*
typedef void* WVec;

WRAPDEF void* __wvresv(__WVHead* vec[static 1], size_t n, size_t stride);
WRAPDEF void* __wvpush(__WVHead* vec[static 1], void* src, size_t n, size_t stride);
WRAPDEF void* __wvpop(__WVHead* vec[static 1], size_t stride);

#define wvresv(vec, n) ((typeof(*(vec))) __wvresv((void*)(vec), (n), sizeof(**(vec))))
#define wvpush(vec, ...) ((typeof(*(vec))) __wvpush((void*)(vec), (typeof(**(vec))[]){__VA_ARGS__}, \
		sizeof((typeof(**(vec))[]){__VA_ARGS__}) / sizeof(**(vec)), sizeof(**(vec))))
#define wvpop(vec) ((typeof(*(vec))) __wvpop((void*)(vec), sizeof(**(vec))))
#define wvec(...) ((typeof(__WA(__VA_ARGS__,))*) __wvpush(&(__WVHead*){NULL}, \
			(typeof(__WA(__VA_ARGS__,))[]){__VA_ARGS__}, sizeof((typeof(__WA(__VA_ARGS__,))[]) \
				{__VA_ARGS__}) / sizeof(__WA(__VA_ARGS__,)), sizeof(__WA(__VA_ARGS__,))))

#define wvbase(vec) ((__WVHead*)(void*)(vec) - ((vec) != NULL))
#define wvhead(vec) ((__WVHead*)(((uintptr_t)(&(__WVHead){0} + 1) & -!(vec)) \
				+ (unsigned char*)(vec)) - 1)
#define wvlen(vec) (wvhead(vec)->size)

#ifndef WRAPNOSTRIP
#define Vec(T) WVec(T)
typedef WVec Vec;
#define resv(vec, n) wvresv(vec, n)
#define push(vec, ...) wvpush(vec, __VA_ARGS__)
#define pop(vec) wvpop(vec)
#define vec(...) wvec(__VA_ARGS__)
#define vbase(vec) wvbase(vec)
#define vhead(vec) wvhead(vec)
#define len(vec) wvlen(vec)
#endif

#ifndef WRAPSTRFSIZE
#define WRAPSTRFSIZE 65536
#endif

typedef WVec(char) WString;

typedef struct {
	size_t len;
	union {
		char* data;
		WString as_owned;
	};
} wstr;

#define was_str(string) ((wstr) { wvlen(string), { string } })
#define wstr(const_lit) ((wstr) { sizeof(const_lit) - 1, { const_lit } })
#define wfmtof(stringlike) \
	_Generic((stringlike), \
			WString: (int) wvlen(whard_cast(String, (stringlike))), \
			wstr: (int) whard_cast(str, (stringlike)).len), \
	_Generic((stringlike), \
			WString: whard_cast(String, (stringlike)), \
			wstr: whard_cast(str, (stringlike)).data)

WRAPDEF bool wstreq(wstr a, wstr b);
WRAPDEF wstr wstrf(WString* string, const char* fmt, ...);
WRAPDEF uint32_t wstrhash32(wstr string);

#ifndef WRAPNOSTRIP
typedef WString String;
typedef wstr str;
#define as_str(string) was_str(string)
#define str(const_lit) wstr(const_lit)
#define fmtof(stringlike) wfmtof(stringlike)
#define streq(a, b) wstreq(a, b)
#define strf(string, ...) wstrf(string, __VA_ARGS__)
#define strhash32(string) wstrhash32(string)
#endif

#ifndef WRAPMAPSIZE
#define WRAPMAPSIZE 16
#endif

#define WMap(V) typeof(Vec(V) (*)[WRAPMAPSIZE])
typedef WMap(void) WMap;
typedef WMap(void) WSet;

WRAPDEF void* __wmput(__WVHead** map[static 1], wstr* keyval, size_t stride);
WRAPDEF void* __wmget(__WVHead** map, wstr key, size_t stride);

#define wmput(map, ...) __wmput((void*)(map), \
		(void*) &(struct { wstr k; __WC(__VA_ARGS__, typeof(****(map)), int,) v; }) \
		{__WAB(__VA_ARGS__, {},)}, __WC(__VA_ARGS__, sizeof(****(map)), 0,))
#define wmget(map, key) (((typeof(**(map))) __wmget((void*)(map), key, sizeof(***(map)))))
#define wmin(map, key) ((bool) __wmget((void*)(map), key, 0))

#ifndef WRAPNOSTRIP
#define Map(V) WMap(V)
typedef WMap Map;
typedef WSet Set;
#define put(map, ...) wmput(map, __VA_ARGS__)
#define get(map, key) wmget(map, key)
#define in(map, key) wmin(map, key)
#endif

#if !defined(WRAPSETIMPL) || defined(WRAPSETIMPL) && defined(WRAPIMPL)

WRAPDEF void* __wvresv(__WVHead* vec[static 1], const size_t n, const size_t stride) {
	__WVHead head = *wvhead(*vec);
	size_t byte_size = head.size + n;
#if defined(__has_builtin) && __has_builtin(__builtin_mul_overflow)
	if(__builtin_mul_overflow(byte_size, stride, &byte_size)
			|| byte_size > ~(~(size_t)0 >> 1) - sizeof(__WVHead)) {
#else
	if(byte_size > SIZE_MAX / stride
			|| (byte_size *= 2) > ~(~(size_t)0 >> 1) - sizeof(__WVHead) {
#endif
#ifdef WRAPNOPANIC
		errno = ERANGE;
		return NULL;
#else
		wpanicf("multiplication overflow on vector\n");
#endif
	}
	if(byte_size > head.byte_capacity) {
#ifndef WRAPREALLOCNOSIZE
		const size_t realloc_size = head.byte_capacity;
#endif
		while((head.byte_capacity = (head.byte_capacity << 1) + sizeof(__WVHead) * 2)
				< byte_size);
		if(!(*vec = WRAPREALLOC(*vec - (*vec != NULL), realloc_size,
						sizeof(__WVHead) + head.byte_capacity))) {
#ifdef WRAPNOPANIC
			return NULL;
#else
			wpanicf("re-allocation failed: %s\n", strerror(errno));
#endif
		}
		*(*vec)++ = head;
	}
	return *vec;
}

WRAPDEF void* __wvpush(__WVHead* vec[static 1], void* const src, const size_t n,
		const size_t stride) {
#ifdef WRAPNOPANIC
	if(!__wvresv(vec, n, stride)) return NULL;
#else
	(void) __wvresv(vec, n, stride);
#endif
	(void) memcpy((unsigned char*)(*vec) + (*vec)[-1].size * stride, src, n * stride);
	(*vec)[-1].size += n;
	return *vec;
}

WRAPDEF void* __wvpop(__WVHead* vec[static 1], const size_t stride) {
	if(!*vec || !(*vec)[-1].size--) {
#ifdef WRAPNOPANIC
		errno = EDOM;
		return NULL;
#else
		wpanicf("called pop on an empty vector\n");
#endif
	}
	return (unsigned char*)(*vec) + (*vec)[-1].size * stride;
}

WRAPDEF bool wstreq(const wstr a, const wstr b) {
	return a.len == b.len && !memcmp(a.data, b.data, a.len);
}

WRAPDEF wstr wstrf(WString* string, const char* fmt, ...) {
	static char buffer[WRAPSTRFSIZE];
	string = (WString*)((unsigned char*)(string) + ((uintptr_t) &(WString){0} & -!(string)));
	va_list va_args;
	va_start(va_args, fmt);
	size_t plen = vsnprintf(buffer, WRAPSTRFSIZE, fmt, va_args);
	if(plen) __wvpush((void*) string, buffer, plen, 1);
	else __wvresv((void*) string, 1, 1);
	return was_str(*string);
}

WRAPDEF uint32_t wstrhash32(wstr string) {
	uint32_t hash = 2166136261u;
	while(string.len--) hash = (hash ^ *string.data++) * 16777619u;
	return hash;
}

WRAPDEF void* __wmput(__WVHead** map[static 1], wstr* keyval, size_t stride) {
	if(!*map) {
		if(!(*map = WRAPREALLOC(NULL, 0, sizeof(void*) * WRAPMAPSIZE))) {
#ifdef WRAPNOPANIC
			return NULL;
#else
			wpanicf("allocation failed: %s\n", strerror(errno));
#endif
		}
		memset(*map, 0, sizeof(void*) * WRAPMAPSIZE);
	}
#ifdef WRAPNOPANIC
	if(!
#endif
	__wvpush(*map + wstrhash32(*keyval) % WRAPMAPSIZE, keyval, 1, sizeof(wstr) + stride)
#ifdef WRAPNOPANIC
	) {
		return NULL;
	}
#else
	;
#endif
	return *map;
}

WRAPDEF void* __wmget(__WVHead** map, wstr key, size_t stride) {
	if(!map) return NULL;
	unsigned char* const vec = (unsigned char*) map[strhash32(key) % WRAPMAPSIZE];
	const uint32_t len = len(vec);
	for(uint32_t i = 0; i < len * (sizeof(wstr) + stride); i += sizeof(wstr) + stride) {
		if(streq(*(wstr*)(vec + i), key)) return vec + i + sizeof(wstr);
	}
	return NULL;
}

#endif

#endif