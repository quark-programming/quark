#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(__has_builtin) && __has_builtin(__builtin_unreachable)
#define unreachable() __builtin_unreachable()
#else
#define unreachable() assert(0 && "Unreachable")
#endif

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

char* fs_readfile(const char* filename);

typedef int Void[0];

#define last(vec) ((vec)[len(vec) - 1])

#endif
