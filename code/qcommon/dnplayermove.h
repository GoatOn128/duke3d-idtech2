/* Shared native player motion; collision remains behind the Pmove trace API. */
#ifndef DN_PLAYER_MOVE_H
#define DN_PLAYER_MOVE_H
#include "../game/q_shared.h"
#include "dnmap.h"

void DN_PlayerMove(pmove_t *move, const dn_map_t *map, dn_prediction_t *state);
void DN_ResetPlayerMotion(dn_prediction_t *state);
#endif
