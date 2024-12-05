#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* exit, malloc          */
#include <stdbool.h>

#include "mc-head.h"
#include "ThreadSafe_PRNG.h"

void* event_worker_thread(WorkerContext* const ctx) {

  const usize event = ctx->event - 1;

  assert(event < countof(PROBABILITY_EVENTS) && "Invalid Event");

  randData rng;

 // ReSharper disable once CppDFALocalValueEscapesFunction
  ctx->rng = &rng;
  ThreadSeedRNG(ctx->rng);

  /*
  Deck* deck = deck_clone(descriptor->deck);
  if (deck == NULL) {
    perror("Failed to clone deck");
    return NULL;
  }
  */

  usize successes = 0;
  for (usize i = 0; i < ctx->iterations; i++) {
    successes += PROBABILITY_EVENTS[event](ctx);
  }

  ctx->successes = successes;

  ctx->rng = NULL;

  return NULL;
}

const Deck* ev_get_shuffled_deck(const WorkerContext* const ctx, usize i) {
  assert(i < ctx->total_decks);
  deck_shuffle(ctx->decks[i], ctx->rng);
  return ctx->decks[i];
}

const Deck* ev_any_shuffled_deck(const WorkerContext* const ctx) {
  return ev_get_shuffled_deck(
    ctx,
    RandomInt(0, (i32)ctx->total_decks - 1, ctx->rng)
  );
}

bool event_pc_1(const WorkerContext* const ctx) {
  const Deck* deck = ev_any_shuffled_deck(ctx);

  const attribute_t needed_suit = card_list_get_attribute(
    deck->reference_list,
    deck->cards[0],
    PC52_ATTR_SUIT
  );

  bool found[5] = {false};

  for (usize i = 0; i < countof(found); i++) {
    const card_id_t card = deck->cards[i];

    const attribute_t suit =
      card_list_get_attribute(deck->reference_list, card, PC52_ATTR_SUIT);
    const attribute_t value =
      card_list_get_attribute(deck->reference_list, card, PC52_ATTR_VALUE);

    if (needed_suit != suit) {
      return false;
    }

    if (value == 1) {
      found[4] = true;
    } else if (value >= 10) {
      found[value - 10] = true;
    } else {
      return false;
    }
  }

  bool found_fold = true;
  for (usize i = 0; i < countof(found); i++) {
    found_fold &= found[i];
  }
  return found_fold;
}

bool event_pc_2(const WorkerContext* const ctx) {
  const Deck* deck = ev_any_shuffled_deck(ctx);

  attribute_t values[PC52_MAX_VALUE] = {0};

  for (usize i = 0; i < 5; i++) {
    const card_id_t card = deck->cards[i];

    const attribute_t value =
      card_list_get_attribute(deck->reference_list, card, PC52_ATTR_VALUE);
    values[value]++;
  }

  for (usize i = 0; i < PC52_MAX_VALUE; i++) {
    if (values[i] >= 4) {
      return true;
    }
  }
  return false;
}

bool event_pc_3(const WorkerContext* const ctx) {
  const Deck* deck = ev_any_shuffled_deck(ctx);

  usize faces_frequencies[3] = {0};
  bool suits_frequencies[4] = {0};

  for (usize i = 0; i < 7; i++) {
    const card_id_t card = deck->cards[i];
    const attribute_t value = deck_get_attribute(deck, card, PC52_ATTR_VALUE);

    const attribute_t suit = deck_get_attribute(deck, card, PC52_ATTR_SUIT);
    suits_frequencies[suit] = true;

    if (value >= 11) {
      faces_frequencies[value - 11]++;
    }
  }

  bool suits_reduce = true;
  for (usize i = 0; i < countof(suits_frequencies); i++) {
    suits_reduce &= suits_frequencies[i];
  }

  if (suits_reduce) {
    return true;
  }

  usize face_pair_count = 0;

  for (usize i = 0; i < countof(faces_frequencies); i++) {
    face_pair_count += faces_frequencies[i] / 2;
  }

  return face_pair_count >= 2;
}

bool event_pc_4(const WorkerContext* const ctx) {
  const Deck* deck = ev_any_shuffled_deck(ctx);

  static const usize NUM_SWITCHES = 30;

  if (deck->total < NUM_SWITCHES) {
    return false;
  }

  usize switch_count = 0;

  bool last_color = deck->cards[0];

  for (usize i = 0; i < deck->total; i++) {
    const card_id_t card = deck->cards[i];
    const bool color = (bool)deck_get_attribute(deck, card, PC52_ATTR_COLOR);

    switch_count += last_color ^ color;

    last_color = color;
  }

  return switch_count >= NUM_SWITCHES;
}

bool event_pc_5(const WorkerContext* const ctx) {
  const Deck* deck = ev_any_shuffled_deck(ctx);

  static const usize NUM_PLAYERS = 3;
  static const usize NUM_CARDS = 3;

  PokemonStage pulled_stage[NUM_PLAYERS];

  for (usize i = 0; i < NUM_PLAYERS; i++) {
    const usize card_index = i * NUM_CARDS;

    pulled_stage[i] = deck_get_attribute( //
      deck,
      deck->cards[card_index],
      POKEMON_ATTR_EVOSTAGE
    );

    for (usize j = 1; j < NUM_CARDS; j++) {
      const card_id_t card = deck->cards[card_index + j];

      const PokemonStage stage =
        deck_get_attribute(deck, card, POKEMON_ATTR_EVOSTAGE);

      if (stage != pulled_stage[i]) {
        return false;
      }
    }
  }

  bool found_stages[POKEMON_STAGE_MAX];

  for (usize i = 0; i < NUM_PLAYERS; i++) {
    found_stages[pulled_stage[i]] = true;
  }

  bool found_reduce = true;
  for (usize i = 0; i < POKEMON_STAGE_MAX; i++) {
    found_reduce &= found_stages[i];
  }

  return found_reduce;
}

/**
 * Draw two cards from each deck.
 * How often will one of the pokemon be unable
 * to attack one of the pokemon drawn
 * by 2 other players
 */
bool event_pc_6(const WorkerContext* const ctx) {
  const Deck* deck = ev_any_shuffled_deck(ctx);

  assert(deck->total >= 4 && "Not enough cards for this event");

  const PokemonType attacker_types[2][2] = {
    {//
     deck_get_attribute(deck, deck->cards[0], POKEMON_ATTR_TYPE1),
     deck_get_attribute(deck, deck->cards[0], POKEMON_ATTR_TYPE2)
    },
    {//
     deck_get_attribute(deck, deck->cards[1], POKEMON_ATTR_TYPE1),
     deck_get_attribute(deck, deck->cards[1], POKEMON_ATTR_TYPE2)
    }
  };

  const PokemonType defender_types[2][2] = {
    {//
     deck_get_attribute(deck, deck->cards[2], POKEMON_ATTR_TYPE1),
     deck_get_attribute(deck, deck->cards[2], POKEMON_ATTR_TYPE2)
    },
    {//
     deck_get_attribute(deck, deck->cards[3], POKEMON_ATTR_TYPE1),
     deck_get_attribute(deck, deck->cards[3], POKEMON_ATTR_TYPE2)
    }
  };

  f32 prod_attack = 1.f;

  for (usize i = 0; i < 2; i++) {

    for (usize j = 0; j < 2; j++) {
      prod_attack *= pokemon_x_attack_y_full(
        attacker_types[i][0],
        attacker_types[i][1],
        defender_types[j][0],
        defender_types[j][1]
      );
    }
  }

  return prod_attack == 0.0f;
}

/* bool event_pc_7(const Deck* deck, randData* data) {} */
