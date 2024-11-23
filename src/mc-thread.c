#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* exit, malloc          */
#include <stdbool.h>

#include "mc-head.h"
#include "ThreadSafe_PRNG.h"

bool event_pc_royal_flush(const Deck* deck, randData* _) {
  const attribute_t needed_suit = card_list_get_attribute(
    deck->reference_list,
    deck->cards[0],
    PC52_ATTR_SUIT
  );

  bool found[5] = {false};

  for (usize i = 0; i < 5; i++) {
    const attribute_t suit = card_list_get_attribute(
      deck->reference_list,
      deck->cards[i],
      PC52_ATTR_SUIT
    );
    const attribute_t value = card_list_get_attribute(
      deck->reference_list,
      deck->cards[i],
      PC52_ATTR_VALUE
    );

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

  for (usize i = 0; i < descriptor->iterations; i++) {
    deck_shuffle(deck, &rng);

    descriptor->successes +=
      PROBABILITY_EVENTS[descriptor->event - 1](deck, &rng);
  }

  deck_free(&deck);

  return NULL;
}
