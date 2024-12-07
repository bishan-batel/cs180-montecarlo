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

#define countof(arr) (sizeof(arr) / sizeof(*(arr)))

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

enum PlayingCardAttribute {
  PC52_ATTR_VALUE,
  PC52_ATTR_COLOR,
  PC52_ATTR_SUIT,
  PC52_ATTR_FACE
};

enum {
  PC52_MAX_VALUE = 13
};

typedef enum {
  POKEMON_ATTR_EVOLINE,
  POKEMON_ATTR_TYPE1,
  POKEMON_ATTR_TYPE2,
  POKEMON_ATTR_EVOSTAGE
} PokemonAttribute;

typedef enum {
  POKEMON_TY_NONE = -1,
  POKEMON_TY_NORMAL = 0,
  POKEMON_TY_FIGHTING,
  POKEMON_TY_FLYING,
  POKEMON_TY_POISON,
  POKEMON_TY_GROUND,
  POKEMON_TY_ROCK,
  POKEMON_TY_BUG,
  POKEMON_TY_GHOST,
  POKEMON_TY_STEEL,
  POKEMON_TY_FIRE,
  POKEMON_TY_WATER,
  POKEMON_TY_GRASS,
  POKEMON_TY_ELECTRIC,
  POKEMON_TY_PSYCHIC,
  POKEMON_TY_ICE,
  POKEMON_TY_DRAGON,
  POKEMON_TY_DARK,
  POKEMON_TY_FAIRY
} PokemonType;

typedef enum {
  POKEMON_STAGE_0 = 0,
  POKEMON_STAGE_1,
  POKEMON_STAGE_2,
  POKEMON_STAGE_MAX
} PokemonStage;

/*
 *  Multiplier  (Effectiveness)             (EXAMPLE, with types)
 *  ****************************************************************
 *  1x          (regular effectiveness)     (grass against fighting)
 *  2x          (super effective!)          (water against fire)
 *  0.25x       (not very effective)        (normal against steel)
 *  0           (immune)                    (ground against flying)
 */
static const f32 POKEMON_MULTIPLIER_LOOKUP[4] = {1, 2, 0.5f, 0};

// The integers in the table below correspond to the hash above
// aka, use the ints from the table to access the floats above
/**
 * 2D Table to get the multipler type between two pokemon types
 *
 * ex.
 *
 * To get the multipler of a fire type attacking a water type,
 *
 * POKEMON_MULTIPLIER_LOOKUP[POKEMON_TYPE_INFO[FIRE_ID][WATER_ID]]
 */
static const usize POKEMON_TYPE_INFO[18][18] = {
  {0, 0, 0, 0, 0, 2, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 0, 2, 2, 0, 1, 2, 3, 1, 0, 0, 0, 0, 2, 1, 0, 1, 2},
  {0, 1, 0, 0, 0, 2, 1, 0, 2, 0, 0, 1, 2, 0, 0, 0, 0, 0},
  {0, 0, 0, 2, 2, 2, 0, 2, 3, 0, 0, 1, 0, 0, 0, 0, 0, 1},
  {0, 0, 3, 1, 0, 1, 2, 0, 1, 1, 0, 2, 1, 0, 0, 0, 0, 0},
  {0, 2, 1, 0, 2, 0, 1, 0, 2, 1, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 2, 2, 2, 0, 0, 0, 2, 2, 2, 0, 1, 0, 1, 0, 0, 1, 2},
  {3, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0},
  {0, 0, 0, 0, 0, 1, 0, 0, 2, 2, 2, 0, 2, 0, 1, 0, 0, 1},
  {0, 0, 0, 0, 0, 2, 1, 0, 1, 2, 2, 1, 0, 0, 1, 2, 0, 0},
  {0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 2, 2, 0, 0, 0, 2, 0, 0},
  {0, 0, 2, 2, 1, 1, 2, 0, 2, 2, 1, 2, 0, 0, 0, 2, 0, 0},
  {0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 1, 2, 2, 0, 0, 2, 0, 0},
  {0, 1, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 3, 0},
  {0, 0, 1, 0, 1, 0, 0, 0, 2, 2, 2, 1, 0, 0, 2, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0, 3},
  {0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 2, 2},
  {0, 1, 0, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 1, 1, 0}
};

f32 pokemon_x_attack_y(PokemonType typeX, PokemonType typeY);

f32 pokemon_x_attack_y_full(
  PokemonType type_x1,
  PokemonType type_x2,
  PokemonType type_y1,
  PokemonType type_y2
);

/**
 * @brief Information about a collection of unique cards
 */
typedef struct {
  usize total;
  usize attributes_per_card;
  attribute_t cards_bytes[];
} CardList;

void card_list_free(CardList** list);

CardList* card_list_from_file(const char* filepath);

attribute_t card_list_get_attribute(
  const CardList* list,
  card_id_t id,
  attribute_id_t attribute_id
);

typedef struct {
  usize total;
  const CardList* reference_list;
  card_id_t cards[];
} Deck;

Deck* deck_from_file(const CardList* reference_list, const char* filepath);

Deck* deck_clone(const Deck* deck);

void deck_shuffle(Deck* deck, randData* rng);

card_id_t deck_pull(const Deck* deck, randData* rng);

void deck_free(Deck** deck);

attribute_t deck_get_attribute(
  const Deck* deck,
  card_id_t id,
  attribute_id_t attribute_id
);

typedef struct {
  usize iterations;
  usize successes;
  event_id_t event;

  usize deck_count;
  const Deck* const* decks;
} WorkerContext;

typedef struct {
  randData* rng;
  usize deck_count;
  Deck* const* decks;
} EventContext;

void* event_worker_thread(WorkerContext*);

static const ThreadFunction EVENT_WORKER_THREAD =
  (ThreadFunction)event_worker_thread;

typedef bool (*ProbabilityEvent)(const EventContext* ctx);

/**
 * 5 Card Pull: How often for a royal flush
 */
bool event_pc_1(const EventContext* ctx);

/**
 * 5 Card Pull: How often is there 4 of a kind
 */
bool event_pc_2(const EventContext* ctx);

/**
 * 7 Card Pull: four suits or two pairs of faces cards
 */
bool event_pc_3(const EventContext* ctx);

/**
 * How often will you encounter at least 30 switches between red and black
 */
bool event_pc_4(const EventContext* ctx);

/**
 * Draw three cards from each deck
 * How often will:
 * one player draws only stage 1
 * one player draws only stage 1
 * one player draws only stage 3
 */
bool event_pc_5(const EventContext* ctx);

/**
 * Draw two cards from each deck.
 * How often will one of the pokemon be unable
 * to attack one of the pokemon drawn
 * by 2 other players
 */
bool event_pc_6(const EventContext* ctx);

/**
 * A starting hand of 7 cards drawn
 *
 */
bool event_pc_7(const EventContext* ctx);

static const ProbabilityEvent PROBABILITY_EVENTS[] = {
  event_pc_1,
  event_pc_2,
  event_pc_3,
  event_pc_4,
  event_pc_5,
  event_pc_6,
  event_pc_7,
};
