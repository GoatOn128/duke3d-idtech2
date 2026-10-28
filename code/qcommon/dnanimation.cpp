/* GAMEDEF.cpp execute(): four 120 Hz clock units per native simulation tick. */
#include "dnanimation.h"
#include <stdlib.h>

void DN_AnimationStep(const dn_action_t *action, int *clock, int *frame, int *count)
{
    *clock += 4;
    if (*clock > action->value[4])
    {
        *clock = 0;
        ++*count;
        *frame += action->value[3];
        if (llabs(*frame) >= llabs((int64_t)action->value[1] * action->value[3]))
        {
            *frame = 0;
        }
    }
}
