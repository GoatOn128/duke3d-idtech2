/* Native destructible props, following GAME.cpp/ACTORS.cpp standable rules. */
#include "dncon.h"

static int TrashCanTile(int tile)
{
    return tile == DN_ConSymbol("CANWITHSOMETHING", 1232) ||
           tile == DN_ConSymbol("CANWITHSOMETHING2", 4580) ||
           tile == DN_ConSymbol("CANWITHSOMETHING3", 4581) ||
           tile == DN_ConSymbol("CANWITHSOMETHING4", 4582);
}

static int CrackTile(int tile)
{
    return tile >= DN_ConSymbol("CRACK1", 546) && tile <= DN_ConSymbol("CRACK4", 549);
}

static int ChargeTile(int tile)
{
    int dead = DN_ConSymbol("SEENINEDEAD", 1248);
    return tile == DN_ConSymbol("SEENINE", 1247) || tile == DN_ConSymbol("OOZFILTER", 1079) ||
           tile == dead || tile == dead + 1;
}

int DN_IsBathroomFixture(int tile)
{
    return tile == DN_ConSymbol("TOILET", 569) || tile == DN_ConSymbol("STALL", 571);
}

int DN_InitDestructible(edict_t *entity)
{
    if (TrashCanTile(entity->tile) || entity->tile == DN_ConSymbol("RUBBERCAN", 1062))
    {
        entity->health = 0;
        entity->clipdist = 72;
        entity->cstat = 257 | (DN_Random() & 4);
        if (!entity->owner)
        {
            entity->owner = entity;
        }
        DN_Fall(entity);
        return 1;
    }
    if (DN_IsBathroomFixture(entity->tile))
    {
        entity->lotag = 1;
        entity->cstat |= 257;
        entity->clipdist = 8;
        entity->owner = entity;
        return 1;
    }
    if (CrackTile(entity->tile))
    {
        entity->cstat |= 17;
        entity->health = 1;
        entity->pal = 0;
        entity->owner = entity;
        entity->actor = -1;
        return 1;
    }
    if (entity->tile == DN_ConSymbol("FANSPRITE", 407) ||
        entity->tile == DN_ConSymbol("GRATE1", 595))
    {
        entity->clipdist = 32;
        entity->cstat |= 257;
        return 1;
    }
    if (entity->tile != DN_ConSymbol("SEENINE", 1247) &&
        entity->tile != DN_ConSymbol("OOZFILTER", 1079))
    {
        return 0;
    }
    entity->shade = -16;
    entity->cstat = entity->xrepeat <= 8 ? 32768 : 257;
    if (entity->xrepeat <= 8)
    {
        entity->xrepeat = entity->yrepeat = 0;
    }
    entity->health = DN_ConSetting("GENERICIMPACTDAMAGE", 10) << 2;
    entity->owner = entity;
    entity->actor = -1;
    return 1;
}

int DN_DamageDestructible(edict_t *entity, int weapon)
{
    if (entity->tile == DN_ConSymbol("GRATE1", 595))
    {
        entity->tile = DN_ConSymbol("BGRATE1", 596);
        entity->cstat &= ~257;
        DN_PlaySound(entity, DN_ConSymbol("VENT_BUST", 18), 0, 0);
        DN_Link(entity);
        return 1;
    }
    if (entity->tile == DN_ConSymbol("BGRATE1", 596))
    {
        return 1;
    }
    if (TrashCanTile(entity->tile))
    {
        entity->hitweapon = 1;
        return 1;
    }
    if (DN_IsBathroomFixture(entity->tile))
    {
        int tile = entity->tile;
        int toilet = tile == DN_ConSymbol("TOILET", 569);
        entity->tile = DN_ConSymbol(toilet ? "TOILETBROKE" : "STALLBROKE", toilet ? 615 : 573);
        entity->cstat |= (DN_Random() & 1) << 2;
        entity->cstat &= ~257;
        edict_t *water = DN_SpawnSprite(entity, DN_ConSymbol("TOILETWATER", 921));
        water->spawnedby = tile;
        DN_PlaySound(entity,
                     DN_ConSymbol(toilet ? "GLASS_BREAKING" : "GLASS_HEAVYBREAK", toilet ? 19 : 20),
                     0, 0);
        DN_Link(entity);
        return 1;
    }
    if (CrackTile(entity->tile))
    {
        if (entity->hitag > 0 &&
            (weapon == DN_ConSymbol("FIREEXT", 916) || weapon == DN_ConSymbol("RPG", 2605) ||
             weapon == DN_ConSymbol("RADIUSEXPLOSION", 1670) ||
             weapon == DN_ConSymbol("SEENINE", 1247) || weapon == DN_ConSymbol("OOZFILTER", 1079)))
        {
            entity->hitweapon = 1;
        }
        return 1;
    }
    if (entity->tile == DN_ConSymbol("FANSPRITE", 407))
    {
        entity->tile = DN_ConSymbol("FANSPRITEBROKE", 411);
        entity->cstat &= ~257;
        if (entity->sector >= 0)
        {
            dn_sector_t *sector = &dn_world.sectors[entity->sector];
            if (sector->floorpicnum == DN_ConSymbol("FANSHADOW", 404))
            {
                sector->floorpicnum = DN_ConSymbol("FANSHADOWBROKE", 405);
                DN_PublishSectorStyle(entity->sector);
            }
        }
        DN_PlaySound(entity, DN_ConSymbol("GLASS_HEAVYBREAK", 20), 0, 0);
        DN_SpawnScrap(entity, 16);
        DN_Link(entity);
        return 1;
    }
    if (entity->tile == DN_ConSymbol("FANSPRITEBROKE", 411))
    {
        return 1;
    }
    if (!ChargeTile(entity->tile))
    {
        return 0;
    }
    if (entity->xrepeat && entity->shade != -32 && entity->shade != -33)
    {
        entity->hitweapon = 1;
    }
    return 1;
}

void DN_TriggerCharges(int sector)
{
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        edict_t *entity = &dn_edicts[i];
        if (entity->inuse && entity->sector == sector &&
            (entity->tile == DN_ConSymbol("SEENINE", 1247) ||
             entity->tile == DN_ConSymbol("OOZFILTER", 1079)))
        {
            entity->shade = -31;
        }
    }
}

static void Detonate(edict_t *entity)
{
    DN_TriggerDemolition(entity->hitag);
    entity->s.origin[2] += 32;
    if ((entity->demolition_direct && entity->xrepeat) || entity->lotag == -99)
    {
        edict_t *effect = DN_SpawnSprite(entity, DN_ConSymbol("EXPLOSION2", 1890));
        int damage = entity->health;
        int args[5] = {DN_ConSetting("SEENINEBLASTRADIUS", 2048), damage >> 2,
                       damage - (damage >> 1), damage - (damage >> 2), damage};
        DN_ConCommand(entity, DN_OP_HITRADIUS, args, 5);
        DN_PlaySound(effect, DN_ConSymbol("PIPEBOMB_EXPLODE", 14), 0, 0);
    }
    if (entity->xrepeat)
    {
        DN_SpawnScrap(entity, 8);
    }
    DN_Remove(entity);
}

int DN_DestructibleTick(edict_t *entity)
{
    if (TrashCanTile(entity->tile))
    {
        DN_Fall(entity);
        if (entity->hitweapon)
        {
            DN_PlaySound(entity, DN_ConSymbol("VENT_BUST", 18), 0, 0);
            DN_SpawnScrap(entity, 10);
            if (entity->lotag > 0)
            {
                DN_SpawnSprite(entity, entity->lotag);
            }
            DN_Remove(entity);
        }
        else
        {
            DN_Link(entity);
        }
        return 1;
    }
    if (CrackTile(entity->tile))
    {
        if (entity->hitweapon && entity->hitag > 0)
        {
            /* Cracks start linked charges at their authored delay, and open
               their SE13 geometry immediately (ACTORS.cpp movestandables). */
            for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
            {
                edict_t *other = &dn_edicts[i];
                if (other->inuse && other->hitag == entity->hitag &&
                    (other->tile == DN_ConSymbol("SEENINE", 1247) ||
                     other->tile == DN_ConSymbol("OOZFILTER", 1079)))
                {
                    other->shade = -32;
                }
            }
            Detonate(entity);
        }
        return 1;
    }
    if (!ChargeTile(entity->tile))
    {
        return 0;
    }
    if (entity->shade == -32)
    {
        if (entity->lotag > 0)
        {
            entity->lotag -= 3;
            if (entity->lotag <= 0)
            {
                entity->lotag = -99;
            }
        }
        else
        {
            entity->shade = -33;
        }
    }
    else if (entity->shade == -33)
    {
        if (!entity->xrepeat)
        {
            Detonate(entity);
            return 1;
        }
        if (++entity->demolition_ticks == 3)
        {
            int dead = DN_ConSymbol("SEENINEDEAD", 1248);
            entity->demolition_ticks = 0;
            if (entity->tile == DN_ConSymbol("OOZFILTER", 1079) || entity->tile == dead + 1)
            {
                Detonate(entity);
                return 1;
            }
            entity->tile = entity->tile == dead ? dead + 1 : dead;
        }
    }
    else if (entity->hitweapon || entity->shade == -31)
    {
        if (entity->hitweapon)
        {
            entity->lotag = 0;
        }
        entity->hitweapon = 0;
        entity->demolition_direct = 1;
        for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
        {
            edict_t *other = &dn_edicts[i];
            if (other->inuse && other->hitag == entity->hitag &&
                (other->tile == DN_ConSymbol("SEENINE", 1247) ||
                 other->tile == DN_ConSymbol("OOZFILTER", 1079)))
            {
                other->shade = -32;
                DN_Link(other);
            }
        }
    }
    DN_Link(entity);
    return 1;
}

void DN_TickDestructibles(void)
{
    /* Original stat-6 insertion puts later map sprites at the list head.
       Preserve this order: a newly armed lower-index charge ticks immediately. */
    for (int i = globals.num_edicts - 1; i > dn_maxclients; i--)
    {
        if (dn_edicts[i].inuse)
        {
            DN_DestructibleTick(&dn_edicts[i]);
        }
    }
}
