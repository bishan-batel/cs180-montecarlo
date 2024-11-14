/* Original code: http://remus.rutgers.edu/~rhoads/Code/random2.c */

/************************************************************************/
/* concatenation of following two 16-bit multiply with carry generators */
/* x(n)=a*x(n-1)+carry mod 2^16 and y(n)=b*y(n-1)+carry mod 2^16,       */
/* number and carry packed within the same 32 bit integer.              */
/************************************************************************/
#include "ThreadSafe_PRNG.h"
#include <pthread.h>
#include <stdlib.h> /* exit, malloc          */
#include <time.h>

// static unsigned int SEED_X = 521288629;
// static unsigned int SEED_Y = 362436069;

unsigned ut_rand(randData* rData) {
  /* Use any pair of non-equal numbers from this list for "a" and "b"
      18000 18030 18273 18513 18879 19074 19098 19164 19215 19584
      19599 19950 20088 20508 20544 20664 20814 20970 21153 21243
      21423 21723 21954 22125 22188 22293 22860 22938 22965 22974
      23109 23124 23163 23208 23508 23520 23553 23658 23865 24114
      24219 24660 24699 24864 24948 25023 25308 25443 26004 26088
      26154 26550 26679 26838 27183 27258 27753 27795 27810 27834
      27960 28320 28380 28689 28710 28794 28854 28959 28980 29013
      29379 29889 30135 30345 30459 30714 30903 30963 31059 31083
  */
  static unsigned int a = 18000, b = 30903;

  rData->SEED_X = a * (rData->SEED_X & 65535) + (rData->SEED_X >> 16);
  rData->SEED_Y = b * (rData->SEED_Y & 65535) + (rData->SEED_Y >> 16);

  return ((rData->SEED_X << 16) + (rData->SEED_Y & 65535));
}

void ut_srand(unsigned seed1, unsigned seed2, randData* rData) {
  if (seed1) {
    rData->SEED_X = seed1; /* use default seeds if parameter is 0 */
  } else {
    rData->SEED_X = 521288629;
  }

  if (seed2) {
    rData->SEED_Y = seed2;
  } else {
    rData->SEED_Y = 362436069;
  }
}

int RandomInt(int low, int high, randData* rData) {
  int r1 = (int)(ut_rand(rData) / 2);
  return r1 % (high - low + 1) + low;
}

void ThreadSeedRNG(randData* rData) {
  unsigned s1 = time(NULL) ^ pthread_self();
  unsigned s2 = time(NULL) ^ (pthread_self() + 100);
  ut_srand(s1, s2, rData);
}

