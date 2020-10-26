#pragma once

#include <stdint.h>
#include <limits.h>
#include <float.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define F32Max FLT_MAX
#define F32Min FLT_MIN
#define F64Max DBL_MAX
#define F64Min DBL_MIN

#define I64Max LONG_MAX
#define I64Min LONG_MIN
#define I32Max INT_MAX
#define I32Min INT_MIN
#define I16Max SHRT_MAX
#define I16Min SHRT_MIN
#define I8Max SCHAR_MAX
#define I8Min SCHAR_MIN

#define U64Max ULONG_MAX
#define U64Min 0
#define U32Max UINT_MAX
#define U32Min 0
#define U16Max USHRT_MAX
#define U16Min 0
#define U8Max UCHAR_MAX
#define U8Min 0

#define BIT(x)    (1u << (x))

#define DEBUG_GPU_DEVICE 1

#if defined(PLATFORM_WINDOWS)
    #define MODULE_EXPORT __declspec(dllexport)
    #define MODULE_IMPORT __declspec(dllimport)
#else
    #define MODULE_EXPORT
    #define MODULE_IMPORT
#endif

