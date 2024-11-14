#include <assert.h>
#include <stdio.h>  /* fopen, fscanf, fclose */
#include <stdlib.h> /* exit, malloc          */
#include <stdbool.h>
#include <pthread.h>
#include "mc-head.h"
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
  // -1 specfically to not screw up first attacking type
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
  free(*list);
  *list = NULL;
}

CardList* card_list_from_file(const char* const filepath) {
  FILE* const file = fopen(filepath, "rt");

  if (!file) {
    fprintf(stderr, "Failed to open file: %s\n", filepath);
    perror("");
    return NULL;
  }

  u32 total = 0;
  u32 attributes_per_card = 0;

  if (fscanf(file, "%u", &total) != 1) { // NOLINT(*cert-err*)
    fprintf(stderr, "Failed to read total unique card count\n");
    return NULL;
  }

  if (fscanf(file, "%u", &attributes_per_card) != 1) { // NOLINT(*cert-err*)
    fprintf(stderr, "Failed to read atrributes per  card\n");
    return NULL;
  }

  assert(total != 0 && "Cannot have an empty cardlist");
  assert(
    attributes_per_card != 0
    && "Card lists must have at least one attribute per card"
  );

  fclose(file);

  CardList* const list = calloc(1, sizeof(CardList));
  *list = (CardList
  ){.total = total,
    .attributes_per_card = attributes_per_card,
    .cards = calloc(total, sizeof(Card))};

  return list;
}

void simulation_get_setup(
  const char* const filename,
  char filepaths[][128],
  u32* const file_count,
  u32* const thread_count,
  u32* const event_number
) {
  // Open the file in text/translated mode
  FILE* const file = fopen(filename, "rt");

  if (file == NULL) {
    fprintf(stderr, "Can't open file: %s\n", filename);
    perror("");
    exit(-1);
  }

  if (fscanf(file, "%u", event_number) != 1) { // NOLINT(*cert-err*)
    fprintf(stderr, "Failed to read event number\n");
    exit(-1);
  }

  if (fscanf(file, "%u", file_count) != 1) { // NOLINT(*cert-err*)
    fprintf(stderr, "Failed to read files number\n");
    exit(-1);
  }

  if (fscanf(file, "%u", thread_count) != 1) { // NOLINT(*cert-err*)
    fprintf(stderr, "Failed to read files number threads\n");
    exit(-1);
  }

  for (usize i = 0; i < *file_count; ++i) {
    fscanf(file, "%s", filepaths[i]);
  }

  fclose(file);
}

void run_simulation(
  const char filepaths[10][128],
  const usize num_files,
  const usize num_threads,
  const usize event_number
) {}

i32 main(const i32 argc, const char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Insufficient parameters supplied\n");
    return -1;
  }

  char filepaths[10][128];

  u32 num_files = (u32)-1;
  u32 num_threads = (u32)-1;
  u32 event_number = (u32)-1;

  simulation_get_setup(
    argv[1],
    filepaths,
    &num_files,
    &num_threads,
    &event_number
  );

  assert(num_files >= 1);
  assert(num_threads >= 0);
  assert(event_number >= 0);

  run_simulation(filepaths, num_files, num_threads, event_number);

  return 0;
}
