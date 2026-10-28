/* Native ACTORS.cpp guts and airborne/landed JIBS simulation. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

void DN_InitGib(edict_t *entity)
{
    static const char *names[] = {"JIBS1",      "JIBS2",      "JIBS3",     "JIBS4",   "JIBS5",
                                  "JIBS6",      "HEADJIB1",   "ARMJIB1",   "LEGJIB1", "LIZMANHEAD1",
                                  "LIZMANARM1", "LIZMANLEG1", "DUKETORSO", "DUKEGUN", "DUKELEG"};
    for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    {
        if (entity->tile == DN_ConSymbol(names[i], -1))
        {
            entity->fragment = 2;
            entity->actor = -1;
            entity->action = entity->animframe = entity->animclock = entity->count = 0;
            entity->cstat = 0;
            return;
        }
    }
}

void DN_SpawnGuts(edict_t *parent, int tile, int count)
{
    int repeat = parent->enemy && parent->xrepeat < 16 ? 8 : 32;
    float floor = DN_PlaneZ(&dn_world, parent->sector, 0, parent->s.origin[0], parent->s.origin[1]);
    float height = fmaxf(parent->s.origin[2] + 8, floor + 8);
    if (parent->tile == DN_ConSymbol("COMMANDER", 1920))
    {
        height += 24;
    }
    for (int i = 0; i < DN_Min(64, count); i++)
    {
        int angle = DN_Random() & 2047;
        edict_t *gib = DN_SpawnSprite(parent, tile);
        gib->shade = -32;
        gib->pal = parent->enemy && parent->pal == 6 ? 6 : 0;
        gib->xrepeat = gib->yrepeat = repeat;
        if (tile == DN_ConSymbol("JIBS2", 2250))
        {
            gib->xrepeat >>= 2;
            gib->yrepeat >>= 2;
        }
        gib->s.angles[YAW] = -angle * 360.f / 2048;
        gib->s.origin[0] = parent->s.origin[0] + ((DN_Random() & 255) - 128) / 16.f;
        gib->s.origin[1] = parent->s.origin[1] - ((DN_Random() & 255) - 128) / 16.f;
        gib->s.origin[2] = height + (DN_Random() & 8191) / 256.f;
        gib->speed = (48 + (DN_Random() & 31)) / 16.f;
        gib->zspeed = (512 + (DN_Random() & 2047)) / 256.f;
        DN_Link(gib);
    }
}

int DN_GibTick(edict_t *entity)
{
    if (entity->fragment != 2)
    {
        return 0;
    }
    entity->speed = fmaxf(0, entity->speed - 1.f / 16);
    if (entity->count++ >= 300 || entity->sector < 0)
    {
        DN_Remove(entity);
        return 1;
    }
    float floor = DN_PlaneZ(&dn_world, entity->sector, 0, entity->s.origin[0], entity->s.origin[1]);
    float ceiling =
        DN_PlaneZ(&dn_world, entity->sector, 1, entity->s.origin[0], entity->s.origin[1]);
    if (floor == ceiling)
    {
        DN_Remove(entity);
        return 1;
    }
    int water = dn_world.sectors[entity->sector].lotag == 2;
    if (entity->s.origin[2] > floor + 2)
    {
        if (entity->animclock < 2)
        {
            ++entity->animclock;
        }
        else if (!water)
        {
            entity->animclock = 0;
            int duke = entity->tile == DN_ConSymbol("DUKELEG", -1) ||
                       entity->tile == DN_ConSymbol("DUKETORSO", -1) ||
                       entity->tile == DN_ConSymbol("DUKEGUN", -1);
            entity->animframe = entity->animframe > (duke ? 6 : 2) ? 0 : entity->animframe + 1;
        }
        if (entity->zspeed > -24)
        {
            if (water)
            {
                entity->zspeed = entity->zspeed > -4 ? entity->zspeed - 48.f / 256 : -4;
            }
            else
            {
                entity->zspeed -= (DN_ConSetting("GRAVITATIONALCONSTANT", 176) - 50) / 256.f;
            }
        }
        int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360);
        int speed = (int)lroundf(entity->speed * 16);
        entity->s.origin[0] += ((speed * DN_Sin(angle + 512)) >> 14) / 16.f;
        entity->s.origin[1] -= ((speed * DN_Sin(angle)) >> 14) / 16.f;
        entity->s.origin[2] += entity->zspeed;
    }
    else
    {
        if (dn_world.sectors[entity->sector].floorstat & 2)
        {
            DN_Remove(entity);
            return 1;
        }
        entity->s.origin[2] = floor + 2;
        entity->speed = 0;
        if (entity->tile == DN_ConSymbol("JIBS6", 2286))
        {
            ++entity->animclock;
            if (!(entity->animclock & 3) && entity->animframe < 7)
            {
                ++entity->animframe;
            }
            if (entity->animclock > 20)
            {
                DN_Remove(entity);
                return 1;
            }
        }
        else
        {
            entity->tile = DN_ConSymbol("JIBS6", 2286);
            entity->animframe = entity->animclock = 0;
        }
    }
    DN_Link(entity);
    return 1;
}

void DN_HitBlood(edict_t *target, edict_t *attacker, const float *impact, int weapon)
{
    if (!target->enemy || weapon == DN_ConSymbol("FREEZEBLAST", 1641) ||
        target->tile == DN_ConSymbol("DRONE", 1880) ||
        target->tile == DN_ConSymbol("ROTATEGUN", 2360) ||
        target->tile == DN_ConSymbol("COMMANDER", 1920) ||
        (target->tile >= DN_ConSymbol("GREENSLIME", 2370) &&
         target->tile <= DN_ConSymbol("GREENSLIME", 2370) + 7))
    {
        return;
    }
    edict_t *blood = DN_SpawnSprite(target, DN_ConSymbol("JIBS6", 2286));
    if (impact)
    {
        VectorCopy(impact, blood->s.origin);
    }
    else
    {
        blood->s.origin[2] += DN_ActorHeight(target) / 2;
    }
    blood->s.origin[2] -= 4;
    blood->pal = 0;
    blood->speed = 1;
    blood->xrepeat = blood->yrepeat = 24;
    blood->s.angles[YAW] = (attacker ? attacker->s.angles[YAW] : target->s.angles[YAW]) -
                           (32 - (DN_Random() & 63)) * 360.f / 2048;
    DN_Link(blood);
    if (gi.cvar("dn_debug_gibs", "0", 0)->value)
    {
        gi.dprintf("Duke hit blood: target %d tile %d repeat %d position %.3f %.3f %.3f\n",
                   target->s.number, blood->tile, blood->xrepeat, blood->s.origin[0],
                   blood->s.origin[1], blood->s.origin[2]);
    }
}
