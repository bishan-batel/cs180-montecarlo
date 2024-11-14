#ifndef PRNG_H
#define PRNG_H

typedef struct
{
    unsigned int SEED_X;
    unsigned int SEED_Y;
} randData;

unsigned ut_rand(randData*);            /* returns a random 32-bit integer */
void ut_srand(unsigned, unsigned, randData* rData); /* seed the generator              */
int RandomInt(int low, int high, randData* rData);  /* range                           */

void ThreadSeedRNG(randData* rData);  //USE THIS PER THREAD

#endif
