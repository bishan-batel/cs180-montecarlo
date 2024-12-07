#include <assert.h>
#include <stdio.h>  /* fopen, fscanf, fclose */
#include <stdlib.h> /* exit, malloc          */
#include <stdbool.h>
#include <pthread.h>
#include "mc-head.h"
#include <string.h>
#include <unistd.h>
#include "ThreadSafe_PRNG.h"

static const usize ITERATION_COUNT = 2500000;

f32 pokemon_x_attack_y(const PokemonType typeX, const PokemonType typeY) {
  // If there's no attacking type, does not contribute to outcome
  // -1 specially to not screw up first attacking type
  if (typeX == POKEMON_TY_NONE) {
    return -1.0f;
  }
  // If there's no defending type, does not contribute to outcome
  // 1 specifically to not screw up first defending type
  if (typeY == POKEMON_TY_NONE) {
    return 1.0f;
  }

  return POKEMON_MULTIPLIER_LOOKUP[POKEMON_TYPE_INFO[typeX][typeY]];
}

f32 pokemon_x_attack_y_full(
  const PokemonType type_x1,
  const PokemonType type_x2,
  const PokemonType type_y1,
  const PokemonType type_y2
) {
  const f32 effect_x1_y =
    pokemon_x_attack_y(type_x1, type_y1) * pokemon_x_attack_y(type_x1, type_y2);

  const f32 effect_x2_y =      //
    type_x2 == POKEMON_TY_NONE //
      ? -1.0f
      : pokemon_x_attack_y(type_x2, type_y1)
          * pokemon_x_attack_y(type_x2, type_y2);

  return effect_x1_y > effect_x2_y ? effect_x1_y : effect_x2_y;
}

// -----------------------------------------------------------------------------
//                               CARD LIST FUNCTIONS
// -----------------------------------------------------------------------------
void card_list_free(CardList** list) {
  assert(list && "Cannot pass a NULL to free");

  if (*list == NULL) {
    return;
  }

  free(*list);
  *list = NULL;
}

CardList* card_list_from_file(const char* const filepath) {
  assert(filepath && "Invalid Filepath");

  FILE* const file = fopen(filepath, "rt");

  if (file == NULL) {
    perror("Failed to open cardfile");
    return NULL;
  }

  usize total = 0;
  usize attributes_per_card = 0;

  if (fscanf(file, "%zu", &total) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read total unique card count\n");
    return NULL;
  }

  if (fscanf(file, "%zu", &attributes_per_card) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read atrributes per  card\n");
    return NULL;
  }

  assert(total != 0 && "Cannot have an empty cardlist");
  assert(
    attributes_per_card != 0
    && "Card lists must have at least one attribute per card"
  );

  CardList* list = calloc(
    1,
    sizeof(CardList) + attributes_per_card * total * sizeof(attribute_t)
  );

  if (list == NULL) {
    perror("Failed to allocate data for card list");
    fclose(file);
    return NULL;
  }

  *list = (CardList){
    .total = total,
    .attributes_per_card = attributes_per_card,
  };

  for (usize i = 0; i < total * attributes_per_card; i++) {
    attribute_t* const attribute_ptr = &list->cards_bytes[i];

    if (fscanf(file, "%u", attribute_ptr) != 1) { // NOLINT(*cert-err*)
      perror("Failed to read attribute");
      fclose(file);
      card_list_free(&list);
      return NULL;
    }
  }

  fclose(file);

  return list;
}

attribute_t card_list_get_attribute(
  const CardList* list,
  const usize id,
  const card_id_t attribute_id
) {
  assert(list && "Invalid list pointer");
  assert(id < list->total && "Invalid Card Index");
  assert(attribute_id < list->attributes_per_card && "Invalid Attribute Index");

  return list->cards_bytes[list->attributes_per_card * id + attribute_id];
}

// -----------------------------------------------------------------------------
//                                DECK FUNCTIONS
// -----------------------------------------------------------------------------

Deck* deck_from_file(const CardList* reference_list, const char* filepath) {
  assert(reference_list && "Invalid Card List");
  assert(filepath && "Invalid Filepath");

  FILE* const file = fopen(filepath, "rt");

  if (file == NULL) {
    fprintf(stderr, "Can't load from %s\n", filepath);
    perror("Failed to load deck file");
    return NULL;
  }

  usize total_cards = 0;

  if (fscanf(file, "%zu", &total_cards) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read atrributes per  card\n");
    return NULL;
  }

  if (total_cards == 0) {
    fprintf(stderr, "Total Cards Cannot be 0 (%s)\n", filepath);
    fclose(file);
    return NULL;
  }

  Deck* deck = calloc(1, sizeof(Deck) + sizeof(card_id_t) * total_cards);

  if (deck == NULL) {
    perror("Failed to allocate data for deck");
    fclose(file);
    return NULL;
  }

  *deck = (Deck){
    .total = total_cards,
    .reference_list = reference_list,
  };

  for (usize i = 0; i < total_cards; i++) {
    if (fscanf(file, "%zu", &deck->cards[i]) != 1) { // NOLINT(*cert-err*)
      perror("Failed to read card ID\n");
      deck_free(&deck);
      fclose(file);
      return NULL;
    }

    // check for invalid card ID
    if (deck->cards[i] >= reference_list->total) {
      fprintf(
        stderr,
        "Invalid Card Id, (%zu >= %zu)",
        deck->cards[i],
        reference_list->total
      );
      deck_free(&deck);
      fclose(file);
      return NULL;
    }
  }

  fclose(file);

  return deck;
}

void deck_free(Deck** deck) {
  assert(deck != NULL && "Invalid Deck Pointer");

  if (*deck != NULL) {
    free(*deck);
    *deck = NULL;
  }
}

void simulation_get_setup(
  const char* const filename,
  char filepaths[][128],
  usize* const file_count,
  usize* const thread_count,
  event_id_t* const event_number
) {
  // Open the file in text/translated mode
  FILE* const file = fopen(filename, "rt");

  if (file == NULL) {
    perror("Can't open file");
    exit(-1);
  }

  if (fscanf(file, "%zu", file_count) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read files number");
    exit(-1);
  }

  if (fscanf(file, "%zu", event_number) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read event number");
    exit(-1);
  }

  if (fscanf(file, "%zu", thread_count) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read files number threads");
    exit(-1);
  }

  for (usize i = 0; i < *file_count; ++i) {
    if (fscanf(file, "%s", filepaths[i]) != 1) { // NOLINT(*cert-err*)
      perror("Failed to read file name");
      exit(-1);
    }
  }

  fclose(file);
}

Deck* deck_clone(const Deck* const deck) {
  assert(deck && "Invalid Deck to clone");

  const usize deck_byte_size = sizeof(Deck) + sizeof(card_id_t) * deck->total;
  Deck* clone = calloc(1, deck_byte_size);

  if (clone == NULL) {
    perror("Failed to allocate data for new Clone Deck");
    return NULL;
  }

  memcpy(clone, deck, deck_byte_size);

  return clone;
}

void deck_shuffle(Deck* const deck, randData* const rng) {
  assert(deck != NULL && "Invalid deck instance");

  for (usize i = 0; i < deck->total; i++) {
    const usize j = RandomInt(0, (i32)deck->total - 1, rng);

    const card_id_t temp = deck->cards[i];
    deck->cards[i] = deck->cards[j];
    deck->cards[j] = temp;
  }
}

card_id_t deck_pull(const Deck* const deck, randData* const rng) {
  assert(deck != NULL && "Invalid deck instance");

  const usize i = RandomInt(0, (i32)deck->total - 1, rng);

  return deck->cards[i];
}

attribute_t deck_get_attribute(
  const Deck* deck,
  const card_id_t id,
  const attribute_id_t attribute_id
) {
  return card_list_get_attribute(deck->reference_list, id, attribute_id);
}

bool run_simulation_threads(
  const event_id_t event,
  const usize thread_count,
  const usize deck_count,
  const Deck* const* const decks
) {
  pthread_t* const thread_handles = calloc(thread_count, sizeof(pthread_t));

  if (thread_handles == NULL) {
    perror("Failed to allocate data for worker threads");
    return false;
  }

  WorkerContext* const threads = calloc(thread_count, sizeof(WorkerContext));

  if (threads == NULL) {
    perror("Failed to allocate data for worker threads");
    return false;
  }

  bool did_fail = false;

  printf("Running event %zu\n", event);
  for (usize i = 0; i < thread_count; i++) {
    WorkerContext* context = &threads[i];

    threads[i] = (WorkerContext){
      .iterations = ITERATION_COUNT,
      .successes = 0,
      .event = event,
      .deck_count = deck_count,
      .decks = decks,
    };

    const errno_t err = pthread_create( //
      &thread_handles[i],
      NULL,
      EVENT_WORKER_THREAD,
      context
    );

    if (err != 0) {
      perror("Failed to create thread");

      // dont make any more threads and go straight to cleanup
      did_fail = true;
      break;
    }
  }

  usize total = 0;
  usize successes = 0;

  for (usize i = 0; i < thread_count; i++) {
    WorkerContext* context = &threads[i];

    if (pthread_join(thread_handles[i], NULL) != 0) {
      perror("Failed to join thread");
      did_fail = true;
      continue;
    }

    total += context->iterations;
    successes += context->successes;
  }

  free(threads);
  free(thread_handles);

  f64 probability = (f64)successes / (f64)total;
  printf("%lf%%\n", 100.f * probability);

  return did_fail;
}

i32 main(const i32 argc, const char* argv[]) {
  if (argc < 2) {
    perror("Insufficient parameters supplied");
    return -1;
  }

  char filepaths[10][128];

  usize num_files = 0;
  usize num_threads = 0;
  event_id_t event_number = 0;

  simulation_get_setup(
    argv[1],
    filepaths,
    &num_files,
    &num_threads,
    &event_number
  );

  assert(num_files >= 2 && "Invalid File Count");
  assert(num_threads >= 1 && "Invalid Thread Count");

  chdir("SimEvents");

  CardList* unique_cards = card_list_from_file(filepaths[0]);

  if (unique_cards == NULL) {
    return -1;
  }

  const usize deck_count = num_files - 1;
  Deck** const all_decks = calloc(num_files - 1, sizeof(Deck*));

  if (all_decks == NULL) {
    perror("Failed to allocate for all decks");
    return -1;
  }

  // read all decks
  for (usize i = 0; i < deck_count; i++) {
    all_decks[i] = deck_from_file(unique_cards, filepaths[i + 1]);

    // early cleanup if the reading failed
    if (all_decks[i] == NULL) {
      for (usize j = 0; j < deck_count; j++) {
        deck_free(&all_decks[i]);
      }
      free(all_decks);

      card_list_free(&unique_cards);
      perror("Failed to create all decks");
      exit(-1);
    }
  }

  run_simulation_threads(
    event_number,
    num_threads,
    deck_count,
    (const Deck* const*)all_decks
  );

  // Free Decks
  for (usize i = 0; i < deck_count; i++) {
    deck_free(&all_decks[i]);
  }
  free(all_decks);

  card_list_free(&unique_cards);
  return 0;
}
