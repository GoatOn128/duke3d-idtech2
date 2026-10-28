/* Server-side actor simulation and runtime CON scheduling. */
#include "dnlocal.h"
#include "dncon.h"

void DN_ActorTick(edict_t *e)
{
    if (e->faller)
    {
        return;
    }
    float previous_z = e->s.origin[2];
    if (DN_VehicleTick(e) || DN_GibTick(e) || DN_FragmentTick(e) || DN_DecalTick(e))
    {
        return;
    }
    if (e->projectile)
    {
        DN_ProjectileTick(e);
        return;
    }
    if (DN_EnemyDormant(e))
    {
        return;
    }
    if (DN_NativeEnemyTick(e) || e->actor < 0)
    {
        return;
    }
    if (e->action > 0 && e->action < 2048)
    {
        DN_AnimationStep(&dn_actions[e->action], &e->animclock, &e->animframe, &e->actioncount);
    }
    if (e->client)
    {
        e->health = e->client->health;
    }
    DN_ConRun(e);
    if (!e->inuse)
    {
        return;
    }
    /* Original execute() evaluates CON before move() increments its counter. */
    e->count++;
    if (e->client)
    {
        e->client->health = e->health;
        e->client->ps.stats[STAT_HEALTH] = e->health;
        DN_Link(e);
        return; /* Player movement remains in the shared predicted Pmove path. */
    }
    DN_MoveActor(e, previous_z);
    if (e->enemy && e->xrepeat <= 60 && e->sleep > 0 &&
        !(e->health <= 0 && gi.cvar("respawn_monsters", "0", 0)->value))
    {
        if (--e->sleep == 0)
        {
            e->dormant = 1;
            e->alert_ticks = 0;
        }
    }
    DN_Link(e);
}
