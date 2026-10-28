/* Game bindings for the shared CON bytecode runtime. */
#include "dncon.h"

void DN_ConInitActor(edict_t *ent)
{
    const dn_actor_t *a = DN_ConActor(ent->tile);
    if (!a)
    {
        ent->actor = -1;
        return;
    }
    ent->count = ent->actioncount = ent->animframe = ent->animclock = 0;
    ent->ai = 0;
    ent->actor = a->code;
    ent->health = a->health;
    ent->action = a->action;
    ent->move = a->move;
    ent->moveflags = a->flags;
    if (a->enemy)
    {
        ent->enemy = 1;
        if (a->enemy == 2)
        {
            ent->stayput = ent->sector;
        }
    }
}

static int Alive(void *state)
{
    return ((edict_t *)state)->inuse;
}

static int Condition(void *state, int op, const int *args, int argc)
{
    return DN_ConCondition(static_cast<edict_t *>(state), op, args, argc);
}

static int Command(void *state, int op, const int *args, int argc)
{
    return DN_ConCommand(static_cast<edict_t *>(state), op, args, argc);
}

void DN_ConRun(edict_t *entity)
{
    dn_con_vm_t vm = {entity, entity->tile, entity->actor, Alive, Condition, Command};
    DN_ConExecute(&vm);
}
