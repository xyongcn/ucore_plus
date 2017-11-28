/*
 * An implementation of Mersenne Twister pseudo randome number generator.
 * Modified from the implementation by Shawn Cokus <Cokus@math.washington.edu>
 * See http://www.mcs.anl.gov/~kazutomo/hugepage-old/twister.c for detail.
 */
#include <types.h>
#include "mersenne_twister_random.h"

#define N              (624)                 // length of state vector
#define M              (397)                 // a period parameter
#define K              (0x9908B0DFU)         // a magic constant
#define hiBit(u)       ((u) & 0x80000000U)   // mask all but highest   bit of u
#define loBit(u)       ((u) & 0x00000001U)   // mask all but lowest    bit of u
#define loBits(u)      ((u) & 0x7FFFFFFFU)   // mask     the highest   bit of u
#define mixBits(u, v)  (hiBit(u)|loBits(v))  // move hi bit of u to hi bit of v

static uint32_t   state[N+1];     // state vector + 1 extra to not violate ANSI C
static uint32_t   *next;          // next random value is computed from here
static int      left = -1;      // can *next++ this many times before reloading

void mersenne_twister_set_seed(uint32_t seed)
{
    uint32_t x = (seed | 1U) & 0xFFFFFFFFU, *s = state;
    int    j;

    for(left=0, *s++=x, j=N; --j;
        *s++ = (x*=69069U) & 0xFFFFFFFFU);
}


static uint32_t mersenne_twister_reload(void)
{
    uint32_t *p0=state, *p2=state+2, *pM=state+M, s0, s1;
    int    j;

    if(left < -1)
        mersenne_twister_set_seed(4357U);

    left=N-1, next=state+1;

    for(s0=state[0], s1=state[1], j=N-M+1; --j; s0=s1, s1=*p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    for(pM=state, j=M; --j; s0=s1, s1=*p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    s1=state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
    s1 ^= (s1 >> 11);
    s1 ^= (s1 <<  7) & 0x9D2C5680U;
    s1 ^= (s1 << 15) & 0xEFC60000U;
    return(s1 ^ (s1 >> 18));
}


uint32_t mersenne_twister_generate(void)
{
    uint32_t y;

    if(--left < 0)
        return(mersenne_twister_reload());

    y  = *next++;
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    return(y ^ (y >> 18));
}
