#pragma once

#include <stdint.h>
#include <stddef.h>
#include <threads.h>

// -----------------------------------------------------------------------------
//                               NUMBER TYPEDEFS
// -----------------------------------------------------------------------------

/**
 * @brief 32 Bit Float
 */
typedef float f32;

/**
 * @brief 64 Bit Float
 */
typedef double f64;

/**
 * @brief 8 bit signed integer
 */
typedef int8_t i8;

/**
 * @brief 16 bit signed integer
 */
typedef int16_t i16;

/**
 * @brief 32 bit signed integer
 */
typedef int32_t i32;

/**
 * @brief 64 bit signed integer
 */
typedef int64_t i64;

/**
 * @brief 8 bit unsigned integer
 */
typedef uint8_t u8;

/**
 * @brief 16 bit unsigned integer
 */
typedef uint16_t u16;

/**
 * @brief 32 bit unsigned integer
 */
typedef uint32_t u32;

/**
 * @brief 64 bit unsigned integer
 */
typedef uint64_t u64;

/**
 * @brief General integer for representing a size / length of data
 */
typedef size_t usize;

// -----------------------------------------------------------------------------
//                                     MC HEAD
// -----------------------------------------------------------------------------

typedef i32 pokemon_type_t;

typedef struct {
} MonteCarloThreadInfo;
