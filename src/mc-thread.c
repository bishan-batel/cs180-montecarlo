#include <stdio.h>
#include <stdlib.h> /* exit, malloc          */
#include <stdbool.h>

#include "mc-head.h"
#include "ThreadSafe_PRNG.h"

void* event_worker_thread(WorkerThreadDescriptor* const descriptor) {
  randData rng;
  ThreadSeedRNG(&rng);

  for (usize i = 0; i < descriptor->iterations; i++) {
    descriptor->successes++;
  }

  return NULL;
}
