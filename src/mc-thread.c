#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* exit, malloc          */
#include <stdbool.h>

#include "mc-head.h"
#include "ThreadSafe_PRNG.h"

enum {
  MAX_CARD_PC_VALUE = 13
};

bool event_pc_event3(const Deck* deck, randData* rng) {
  for (usize i = 0; i < 7; i++) {
    const card_id_t card = deck->cards[i];
  }
  return false;
}

bool event_pc_four_of_a_kind(const Deck* deck, randData* rng) {
  attribute_t values[MAX_CARD_PC_VALUE] = {0};

  for (usize i = 0; i < 5; i++) {
    const card_id_t card = deck->cards[i];

    const attribute_t value =
      card_list_get_attribute(deck->reference_list, card, PC52_ATTR_VALUE);
    values[value]++;
  }

  for (usize i = 0; i < MAX_CARD_PC_VALUE; i++) {
    if (values[i] >= 4) {
      return true;
    }
  }
  return false;
}

bool event_pc_royal_flush(const Deck* deck, randData* rng) {
  const attribute_t needed_suit = card_list_get_attribute(
    deck->reference_list,
    deck->cards[0],
    PC52_ATTR_SUIT
  );

  bool found[5] = {false};

  for (usize i = 0; i < 5; i++) {
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
  for (usize i = 0; i < 5; i++) {
    found_fold &= found[i];
  }
  return found_fold;
}

void* event_worker_thread(WorkerThreadDescriptor* const descriptor) {
  randData rng;
  ThreadSeedRNG(&rng);

  assert(
    descriptor->event - 1
      < sizeof(PROBABILITY_EVENTS) / sizeof(*PROBABILITY_EVENTS)
    && "Invalid Event"
  );

  Deck* deck = deck_clone(descriptor->deck);

  if (deck == NULL) {
    perror("Failed to clone deck");
    return NULL;
  }

  usize successes = 0;
  for (usize i = 0; i < descriptor->iterations; i++) {
    deck_shuffle(deck, &rng);

    /* successes += PROBABILITY_EVENTS[descriptor->event - 1](deck, &rng); */
    successes += RandomInt(0, 1, &rng);
  }

  deck_free(&deck);
  descriptor->successes = successes;

  return NULL;
}
