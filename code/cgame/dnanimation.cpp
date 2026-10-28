/* Predict only the active action's visual clock. CON transitions stay authoritative. */
#include "cgame.h"

int DN_PredictAnimation(const entity_state_t *state, uint32_t target_tick)
{
    const dn_animation_t *sample = &state->animation;
    if (!(state->renderfx & RF_DUKE_SPRITE) || !(sample->flags & DN_ANIMATION_RUNNING) ||
        sample->action <= 0 || sample->action >= 2048 || sample->tile < 0 ||
        sample->tile >= DN_MAX_TILES || sample->clock < 0 || sample->clock > 1000000 ||
        sample->frame < -DN_MAX_TILES || sample->frame > DN_MAX_TILES)
    {
        return state->frame;
    }
    int32_t ticks = (int32_t)(target_tick - sample->tick);
    if (ticks <= 0)
    {
        return state->frame;
    }
    if (ticks > DN_ANIMATION_MAX_PREDICTION)
    {
        ticks = DN_ANIMATION_MAX_PREDICTION;
    }
    const dn_action_t *action = &dn_actions[sample->action];
    int clock = sample->clock, frame = sample->frame, count = 0;
    if (action->value[1] < -DN_MAX_TILES || action->value[1] > DN_MAX_TILES ||
        action->value[3] < -DN_MAX_TILES || action->value[3] > DN_MAX_TILES ||
        action->value[2] < 0 || action->value[2] > 8)
    {
        return state->frame;
    }
    for (int tick = 0; tick < ticks; tick++)
    {
        DN_AnimationStep(action, &clock, &frame, &count);
    }
    int64_t tile = (int64_t)sample->tile + action->value[0] + frame * action->value[2];
    return tile >= 0 && tile < DN_MAX_TILES ? (int)tile : state->frame;
}
