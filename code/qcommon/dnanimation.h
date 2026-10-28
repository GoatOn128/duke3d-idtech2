/* Authoritative CON action clock and deterministic presentation-only stepping. */
#ifndef DN_ANIMATION_H
#define DN_ANIMATION_H
#include "dncon.h"
#include <stdint.h>

#define DN_ANIMATION_RUNNING 1
#define DN_ANIMATION_MAX_PREDICTION 6

typedef struct
{
    uint32_t tick;
    int32_t tile, action, frame, clock, flags;
} dn_animation_t;

void DN_AnimationStep(const dn_action_t *action, int *clock, int *frame, int *count);
#endif
