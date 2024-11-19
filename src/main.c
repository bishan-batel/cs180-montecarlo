#include <assert.h>
#include <stdio.h>  /* fopen, fscanf, fclose */
#include <stdlib.h> /* exit, malloc          */
#include <stdbool.h>
#include <pthread.h>
#include "mc-head.h"
#include <unistd.h>
#include "ThreadSafe_PRNG.h"

enum PC52_ATT {
  PC52_value,
  PC52_color,
  PC52_suit,
  PC52_face
};

enum PKMN_ATT {
  PKMN_evoline,
  PKMN_type1,
  PKMN_type2,
  PKMN_evostage
};

//  Multiplier  (Effectiveness)             (EXAMPLE, with types)
//  ****************************************************************
//  1x          (regular effectiveness)     (grass against fighting)
//  2x          (super effective!)          (water against fire)
//  0.25x       (not very effective)        (normal against steel)
//  0           (immune)                    (ground against flying)

static const f32 PKMN_TypeChartHash[4] = {1, 2, 0.5f, 0};

// The integers in the table below correspond to the hash above
// aka, use the ints from the table to access the floats above
static const usize PKMN_TypeChart[18][18] = {
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

f32 typechart_x_attack_y(
  const pokemon_type_t typeX,
  const pokemon_type_t typeY
) {
  assert(typeX >= -1 && "Invalid Type X");
  assert(typeY >= -1 && "Invalid Type Y");

  // If there's no attacking type, does not contribute to outcome
  // -1 specially to not screw up first attacking type
  if (typeX == -1) {
    return -1.0f;
  }
  // If there's no defending type, does not contribute to outcome
  // 1 specifically to not screw up first defending type
  if (typeY == -1) {
    return 1.0f;
  }

  return PKMN_TypeChartHash[PKMN_TypeChart[typeX][typeY]];
}

f32 typechart_x_attack_y_full(
  const pokemon_type_t type_x1,
  const pokemon_type_t type_x2,
  const pokemon_type_t type_y1,
  const pokemon_type_t type_y2
) {
  const f32 effect_x1_y = typechart_x_attack_y(type_x1, type_y1)
                        * typechart_x_attack_y(type_x1, type_y2);

  const f32 effect_x2_y = //
    type_x2 == -1         //
      ? -1.0f
      : typechart_x_attack_y(type_x2, type_y1)
          * typechart_x_attack_y(type_x2, type_y2);

  return effect_x1_y > effect_x2_y ? effect_x1_y : effect_x2_y;
}

void card_list_free(CardList** list) {
  assert(list && "Cannot pass a NULL to free");

  if (*list == NULL) {
    return;
  }

  free(*list);
  *list = NULL;
}

const CardList* card_list_from_file(const char* const filepath) {
  assert(filepath && "Invalid Filepath");

  FILE* const file = fopen(filepath, "rt");

  if (!file) {
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

  if (!list) {
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

const Deck* read_deck(const CardList* reference_list, const char* filepath) {}

void simulation_get_setup(
  const char* const filename,
  char filepaths[][128],
  usize* const file_count,
  usize* const thread_count,
  usize* const event_number
) {
  // Open the file in text/translated mode
  FILE* const file = fopen(filename, "rt");

  if (file == NULL) {
    perror("Can't open file");
    exit(-1);
  }

  if (fscanf(file, "%zu", event_number) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read event number");
    exit(-1);
  }

  if (fscanf(file, "%zu", file_count) != 1) { // NOLINT(*cert-err*)
    perror("Failed to read files number");
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

i32 main(const i32 argc, const char* argv[]) {
  if (argc < 2) {
    perror("Insufficient parameters supplied");
    return -1;
  }

  char filepaths[10][128];

  usize num_files = 0;
  usize num_threads = 0;
  usize event_number = 0;

  simulation_get_setup(
    argv[1],
    filepaths,
    &num_files,
    &num_threads,
    &event_number
  );

  assert(num_files >= 1);
  assert(num_threads >= 1);
  assert(event_number >= 0);

  chdir("SimEvents");

  CardList* unique_cards = card_list_from_file(filepaths[0]);

  if (!unique_cards) {
    return -1;
  }

  for (usize i = 0; i < unique_cards->total; i++) {
    printf("%zu: ", i);
    for (usize j = 0; j < unique_cards->attributes_per_card; j++) {
      printf("%d, ", card_list_get_attribute(unique_cards, i, j));
    }
    printf("\n");
  }

  card_list_free(&unique_cards);
  return 0;
}

Deck* deck_from_file(const CardList* reference_list, const char* filepath) {}
