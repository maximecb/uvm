#ifndef __STDINT_H__
#define __STDINT_H__

// Signed integer types
#define int8_t i8
#define int16_t i16
#define int32_t i32
#define int64_t i64

// Unsigned integer types
#define uint8_t u8
#define uint16_t u16
#define uint32_t u32
#define uint64_t u64

// Unsigned integer ranges
#define UINT8_MAX  0xFF
#define UINT16_MAX 0xFFFF
#define UINT32_MAX 0xFFFFFFFF
#define UINT64_MAX 0xFFFFFFFFFFFFFFFF

// Signed integer ranges
#define INT8_MIN  0x80
#define INT8_MAX  0x7F
#define INT16_MIN 0x8000
#define INT16_MAX 0x7FFF
#define INT32_MIN 0x80000000
#define INT32_MAX 0x7FFFFFFF
#define INT64_MIN 0x8000000000000000
#define INT64_MAX 0x7FFFFFFFFFFFFFFF

#endif
