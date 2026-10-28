/* Server operation bindings for the shared CON runtime. */
#ifndef DN_GAME_CON_H
#define DN_GAME_CON_H
#include "dnlocal.h"
#include "../qcommon/dncon.h"
void DN_ConInitActor(edict_t *entity);
void DN_ConRun(edict_t *entity);
int DN_ConCondition(edict_t *entity, int op, const int *args, int argc);
int DN_ConCommand(edict_t *entity, int op, const int *args, int argc);
#endif
