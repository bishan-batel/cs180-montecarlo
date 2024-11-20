#pragma once

#include <stdint.h>
#include <stddef.h>
#include <threads.h>
#include <stdbool.h>
#include "ThreadSafe_PRNG.h"

// -----------------------------------------------------------------------------
//                               NUMBER TYPEDEFS
//                  (this was pulled from our teams GAM100 Project)
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

/**
 * @brief STL Error Type
 */
typedef int errno_t;

/**
 * @brief Event ID
 */
typedef usize event_id_t;

/**
 * @brief Pokemon Type ID
 */
typedef i32 pokemon_id_t;

/**
 * @brief Card Attribute Value
 */
typedef i32 attribute_t;

/**
 * @brief Card Attribute ID Value
 *
 * Used in tandem with card_id_t to get the value of a
 * given attribute of a given card
 */
typedef usize attribute_id_t;

/**
 * @brief Unique Card ID (used to index a Card List for an attribute)
 */
typedef usize card_id_t;

/**
 * @brief Function Pointer type for use with pthreads
 */
typedef void* (*ThreadFunction)(void*);

/**
 * @brief Information
 */
typedef struct {
  usize total;
  usize attributes_per_card;
  attribute_t cards_bytes[];
} CardList;

typedef struct {
  usize total;
  const CardList* reference_list;
  card_id_t cards[];
} Deck;

typedef struct {
  const Deck* deck;
  usize iterations;
  usize successes;
  event_id_t event;
} WorkerThreadDescriptor;

void* event_worker_thread(WorkerThreadDescriptor*);

static const ThreadFunction EVENT_WORKER_THREAD =
  (ThreadFunction)event_worker_thread;

void card_list_free(CardList** list);

CardList* card_list_from_file(const char* filepath);

attribute_t card_list_get_attribute(
  const CardList* list,
  card_id_t id,
  attribute_id_t attribute_id
);

Deck* deck_from_file(const CardList* reference_list, const char* filepath);

Deck* deck_clone(const Deck* deck);

void deck_shuffle(randData* rng, Deck* deck);

void deck_free(Deck** deck);
