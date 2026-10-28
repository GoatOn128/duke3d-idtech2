#ifndef DN_RANDOM_H
#define DN_RANDOM_H
#include <stdint.h>

/* Each replay owns its RNG state. Rewinding restores this alongside actor state. */
static inline int DN_RandomNext(uint32_t *state)
{
    *state = *state * 27584621u + 1u;
    return *state >> 16;
}
#endif
