/* Native effect initialization and scrap motion from GAME.cpp/ACTORS.cpp. */
#include "dncon.h"

void DN_InitEffect(edict_t *entity, edict_t *parent)
{
    if (entity->tile == DN_ConSymbol("TOILETWATER", 921))
    {
        entity->shade = -16;
        return;
    }
    if (parent && entity->tile == DN_ConSymbol("TRANSPORTERBEAM", 1261))
    {
        entity->xrepeat = 31;
        entity->yrepeat = 1;
        entity->shade = -127;
        entity->cstat = 128 | 2;
        entity->speed = 128.f / 16;
        entity->s.origin[2] = dn_world.sectors[parent->sector].floorz / -256.f + 40;
        float angle = parent->s.angles[YAW] * (float)M_PI / 180;
        entity->s.origin[0] += cosf(angle) * entity->speed;
        entity->s.origin[1] += sinf(angle) * entity->speed;
        return;
    }
    int explosion = entity->tile == DN_ConSymbol("EXPLOSION2", 1890) ||
                    entity->tile == DN_ConSymbol("EXPLOSION2BOT", 2219);
    int smoke = entity->tile == DN_ConSymbol("SMALLSMOKE", 2329);
    int burning = entity->tile == DN_ConSymbol("BURNING", 2271) ||
                  entity->tile == DN_ConSymbol("BURNING2", 2310);
    int shrink = entity->tile == DN_ConSymbol("SHRINKEREXPLOSION", 1647);
    if (!explosion && !smoke && !burning && !shrink)
    {
        return;
    }
    if (parent)
    {
        entity->shade = -64;
        entity->cstat = 128 | (DN_Random() & 4);
    }
    entity->xrepeat = entity->yrepeat = explosion ? 48 : smoke ? 24 : burning ? 4 : 32;
    if (explosion)
    {
        entity->shade = -127;
        entity->cstat |= 128;
    }
    if (parent && entity->sector >= 0)
    {
        float floor =
            DN_PlaneZ(&dn_world, entity->sector, 0, entity->s.origin[0], entity->s.origin[1]);
        entity->s.origin[2] = fmaxf(entity->s.origin[2], floor + 12);
    }
}

void DN_SpawnScrap(edict_t *parent, int count)
{
    for (int i = 0; i < count; i++)
    {
        edict_t *scrap = DN_SpawnSprite(parent, DN_ConSymbol("SCRAP6", 2390) + (DN_Random() & 15));
        scrap->fragment = 1;
        scrap->actor = -1;
        scrap->shade = -8;
        scrap->cstat = 0;
        scrap->xrepeat = scrap->yrepeat = 48;
        scrap->s.origin[0] += ((DN_Random() & 255) - 128) / 16.f;
        scrap->s.origin[1] -= ((DN_Random() & 255) - 128) / 16.f;
        scrap->s.origin[2] += 8 + (DN_Random() & 8191) / 256.f;
        scrap->s.angles[1] = -(DN_Random() & 2047) * 360.f / 2048;
        scrap->speed = ((DN_Random() & 63) + 64) / 16.f;
        scrap->zspeed = (512 + (DN_Random() & 2047)) / 256.f;
        DN_Link(scrap);
    }
}

int DN_FragmentTick(edict_t *entity)
{
    if (entity->fragment == 3)
    {
        return DN_GlassTick(entity);
    }
    if (!entity->fragment)
    {
        return 0;
    }
    if (entity->sector < 0)
    {
        DN_Remove(entity);
        return 1;
    }
    entity->speed = fmaxf(0, entity->speed - 1.f / 16);
    float floor = DN_PlaneZ(&dn_world, entity->sector, 0, entity->s.origin[0], entity->s.origin[1]);
    if (entity->s.origin[2] <= floor + 2)
    {
        DN_Remove(entity);
        return 1;
    }
    if (++entity->animclock > 1)
    {
        entity->animclock = 0;
        int frames = entity->tile < DN_ConSymbol("SCRAP6", 2390) + 8 ? 8 : 4;
        entity->animframe = (entity->animframe + 1) % frames;
    }
    if (entity->zspeed > -16)
    {
        entity->zspeed -= (DN_ConSetting("GRAVITATIONALCONSTANT", 176) - 50) / 256.f;
    }
    float angle = entity->s.angles[1] * (float)M_PI / 180;
    entity->s.origin[0] += cosf(angle) * entity->speed;
    entity->s.origin[1] += sinf(angle) * entity->speed;
    entity->s.origin[2] += entity->zspeed;
    DN_Link(entity);
    return 1;
}
