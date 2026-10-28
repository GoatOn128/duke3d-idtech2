/* GAME.cpp stat-12 map props and ACTORS.cpp movefallers(). */
#include "dncon.h"
#include "../qcommon/dnmath.h"

int DN_InitFaller(edict_t *entity)
{
    static const char *exceptions[] = {"SPEAKER",  "LETTER",     "DUCK",       "TARGET",
                                       "TRIPBOMB", "VIEWSCREEN", "VIEWSCREEN2"};
    int crack = DN_ConSymbol("CRACK1", 546);
    if (!(entity->cstat & 48) || !entity->hitag || entity->shade == 127 ||
        (entity->tile >= crack && entity->tile <= DN_ConSymbol("CRACK4", 549)) ||
        DN_IsWallSwitch(entity->tile))
    {
        return 0;
    }
    for (int i = 0; i < sizeof(exceptions) / sizeof(exceptions[0]); i++)
    {
        if (entity->tile == DN_ConSymbol(exceptions[i], -1))
        {
            return 0;
        }
    }
    entity->faller = 1;
    entity->actor = -1;
    entity->cstat |= 257;
    entity->health = DN_ConSetting("GENERICIMPACTDAMAGE", 10);
    return 1;
}

int DN_DamageFaller(edict_t *entity, int damage, int weapon)
{
    if (!entity->faller)
    {
        return 0;
    }
    if (entity->faller == 1 && entity->tile == DN_ConSymbol("FANSPRITE", 407) &&
        weapon == DN_ConSymbol("SHOTSPARK1", 2595))
    {
        /* A bullet opens the fan immediately. Its tagged frame still belongs
           to the later demolition group, rather than becoming permanent. */
        return DN_DamageDestructible(entity, weapon);
    }
    if (entity->faller != 1 ||
        (weapon != DN_ConSymbol("FIREEXT", 916) && weapon != DN_ConSymbol("RPG", 2605) &&
         weapon != DN_ConSymbol("RADIUSEXPLOSION", 1670) &&
         weapon != DN_ConSymbol("SEENINE", 1247) && weapon != DN_ConSymbol("OOZFILTER", 1079)))
    {
        return 1;
    }
    entity->health -= damage;
    if (entity->health <= 0)
    {
        for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
        {
            edict_t *other = &dn_edicts[i];
            if (other->inuse && other->faller && other->hitag == entity->hitag)
            {
                other->faller = 2;
                other->cstat &= ~64;
                if (other->tile == DN_ConSymbol("CEILINGSTEAM", 1250) ||
                    other->tile == DN_ConSymbol("STEAM", 1255))
                {
                    other->cstat |= 32768;
                }
                DN_Link(other);
            }
        }
    }
    return 1;
}

void DN_TickFallers(void)
{
    for (int i = globals.num_edicts - 1; i > dn_maxclients; i--)
    {
        edict_t *entity = &dn_edicts[i];
        if (!entity->inuse || entity->faller != 2 || entity->sector < 0)
        {
            continue;
        }
        if (entity->lotag > 0)
        {
            entity->lotag -= 3;
            if (entity->lotag <= 0)
            {
                entity->speed = ((32 + DN_Random()) & 63) / 16.f;
                entity->zspeed = (1024 + (DN_Random() & 1023)) / 256.f;
            }
            continue;
        }
        if (entity->speed > 0)
        {
            entity->speed -= 8.f / 16;
            int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
            int speed = (int)lroundf(entity->speed * 16);
            vec3_t end;
            VectorCopy(entity->s.origin, end);
            end[0] += ((speed * DN_Sin(angle + 512)) >> 14) / 32.f;
            end[1] -= ((speed * DN_Sin(angle)) >> 14) / 32.f;
            vec3_t mins = {-1, -1, -4}, maxs = {1, 1, 4};
            trace_t trace = gi.trace(entity->s.origin, mins, maxs, end, entity, MASK_SOLID);
            VectorCopy(trace.endpos, entity->s.origin);
            entity->s.origin[2] += entity->zspeed / 2;
        }
        int gravity = DN_ConSetting("GRAVITATIONALCONSTANT", 176);
        if (DN_ConCondition(entity, DN_OP_IFINOUTERSPACE, NULL, 0))
        {
            gravity = 0;
        }
        else if (DN_ConCondition(entity, DN_OP_IFINSPACE, NULL, 0))
        {
            gravity /= 6;
        }
        float floor = -dn_world.sectors[entity->sector].floorz / 256.f;
        if (entity->s.origin[2] > floor + 1)
        {
            int velocity = DN_Min(6144, (int)lroundf(-entity->zspeed * 256) + gravity);
            entity->zspeed = -velocity / 256.f;
            entity->s.origin[2] += entity->zspeed;
        }
        if (entity->s.origin[2] - floor < 16)
        {
            DN_SpawnScrap(entity, 1 + (DN_Random() & 7));
            DN_Remove(entity);
        }
        else
        {
            DN_Link(entity);
        }
    }
}
