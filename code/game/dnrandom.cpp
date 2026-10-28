/* Build's 32-bit krand sequence belongs to the authoritative simulation. */
#include "dnlocal.h"
#include "../qcommon/dnrandom.h"

static uint32_t random_state = 17;

void DN_SeedRandom(uint32_t seed)
{
    random_state = seed;
}

uint32_t DN_RandomState(void)
{
    return random_state;
}

int DN_Random(void)
{
    return DN_RandomNext(&random_state);
}
