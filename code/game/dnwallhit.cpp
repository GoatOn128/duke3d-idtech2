/* Native masked glass walls and GLASSPIECES debris (SECTOR.cpp/ACTORS.cpp). */
#include "dncon.h"

static void SpawnGlass(edict_t *attacker, int index, const float *impact, int coloured)
{
    const dn_wall_t *wall = &dn_world.walls[index];
    const dn_wall_t *other = &dn_world.walls[wall->point2];
    int count = coloured ? 80 : 10;
    edict_t *sound = NULL;
    for (int i = 0; i < count; i++)
    {
        edict_t *piece = DN_SpawnSprite(attacker, DN_ConSymbol("GLASSPIECES", 1031) + i % 3);
        piece->fragment = 3;
        piece->actor = -1;
        piece->action = piece->animframe = piece->animclock = 0;
        piece->shade = -32;
        piece->cstat = 0;
        piece->xrepeat = piece->yrepeat = 36;
        piece->pal = coloured ? DN_Random() & 15 : 0;
        float along = (i + 1.f) / (count + 1.f);
        piece->s.origin[0] = (wall->x + (other->x - wall->x) * along) / 16;
        piece->s.origin[1] = -(wall->y + (other->y - wall->y) * along) / 16;
        /* Keep fragments just inside the struck side of the opening. */
        float length = hypotf(other->x - (float)wall->x, other->y - (float)wall->y);
        if (length > 0)
        {
            piece->s.origin[0] -= (other->y - wall->y) / length / 16;
            piece->s.origin[1] -= (other->x - wall->x) / length / 16;
        }
        piece->sector = DN_FindSectorXY(&dn_world, piece->s.origin, attacker->sector);
        if (piece->sector < 0)
        {
            DN_Remove(piece);
            continue;
        }
        float floor =
            DN_PlaneZ(&dn_world, piece->sector, 0, piece->s.origin[0], piece->s.origin[1]);
        float ceiling =
            DN_PlaneZ(&dn_world, piece->sector, 1, piece->s.origin[0], piece->s.origin[1]);
        piece->s.origin[2] = fminf(
            ceiling - 4, fmaxf(floor + 4, impact[2] + ((DN_Random() & 16383) - 8192) / 256.f));
        piece->s.angles[YAW] = attacker->s.angles[YAW] + 180;
        piece->speed = (32 + (DN_Random() & 63)) / 16.f;
        piece->zspeed = (DN_Random() & 1023) / 256.f;
        DN_Link(piece);
        sound = piece;
    }
    DN_PlaySound(sound ? sound : attacker, DN_ConSymbol("GLASS_BREAKING", 19), 0, 0);
}

int DN_DamageWall(edict_t *attacker, const float *start, const float *end, const trace_t *trace)
{
    if (!attacker || trace->fraction >= 1 || trace->startsolid ||
        (trace->ent && trace->ent != dn_edicts))
    {
        return 0;
    }
    /* Recover the exact native wall without extending the engine/game trace ABI.
       Reuse the original ray: trace endpoints are backed away from the surface. */
    int index;
    trace_t native =
        DN_TraceWall(&dn_world, start, end, vec3_origin, vec3_origin, MASK_SHOT, &index);
    if (index < 0 || fabsf(native.fraction - trace->fraction) > .00001f)
    {
        return 0;
    }
    dn_wall_t *wall = &dn_world.walls[index];
    int coloured = wall->overpicnum == DN_ConSymbol("STAINGLASS1", 510);
    if (!(wall->cstat & 16) || wall->nextsector < 0 ||
        (!coloured && wall->overpicnum != DN_ConSymbol("GLASS", 503)))
    {
        return 0;
    }
    /* A portal's upper/lower solid wall can share the glass texture. */
    float floor = DN_PlaneZ(&dn_world, wall->nextsector, 0, native.endpos[0], native.endpos[1]);
    float ceiling = DN_PlaneZ(&dn_world, wall->nextsector, 1, native.endpos[0], native.endpos[1]);
    if (native.endpos[2] <= floor || native.endpos[2] >= ceiling)
    {
        return 0;
    }
    wall->cstat = 0;
    if (!coloured)
    {
        wall->overpicnum = DN_ConSymbol("GLASS2", 504);
    }
    DN_PublishWallStyle(index);
    if (wall->nextwall >= 0)
    {
        dn_wall_t *back = &dn_world.walls[wall->nextwall];
        back->cstat = 0;
        if (!coloured)
        {
            back->overpicnum = wall->overpicnum;
        }
        DN_PublishWallStyle(wall->nextwall);
    }
    SpawnGlass(attacker, index, native.endpos, coloured);
    if (gi.cvar("dn_debug_hitscan", "0", 0)->value)
    {
        gi.dprintf("Duke glass break: wall %d back %d shards %d\n", index, wall->nextwall,
                   coloured ? 80 : 10);
    }
    return 1;
}

int DN_GlassTick(edict_t *entity)
{
    if (entity->sector < 0 || ++entity->count > 300)
    {
        DN_Remove(entity);
        return 1;
    }
    float floor = DN_PlaneZ(&dn_world, entity->sector, 0, entity->s.origin[0], entity->s.origin[1]);
    entity->zspeed =
        fmaxf(-16, entity->zspeed - DN_ConSetting("GRAVITATIONALCONSTANT", 176) / 256.f);
    if (entity->s.origin[2] + entity->zspeed <= floor + 4)
    {
        if (entity->inventory == 3)
        {
            DN_Remove(entity);
            return 1;
        }
        entity->s.origin[2] = floor + 4;
        entity->zspeed = (3 - entity->inventory) + (DN_Random() & 511) / 256.f;
        if (dn_world.sectors[entity->sector].lotag == 2)
        {
            entity->zspeed /= 2;
        }
        entity->xrepeat >>= 1;
        entity->yrepeat >>= 1;
        entity->inventory++;
    }
    else
    {
        entity->s.origin[2] += entity->zspeed;
    }
    entity->speed = fmaxf(0, entity->speed - 2.f / 16);
    entity->cstat = (((int)lroundf(entity->speed * 16)) & 3) << 2;
    float angle = entity->s.angles[YAW] * (float)M_PI / 180;
    vec3_t end;
    VectorSet(end, entity->s.origin[0] + cosf(angle) * entity->speed,
              entity->s.origin[1] + sinf(angle) * entity->speed, entity->s.origin[2]);
    trace_t trace =
        DN_Trace(&dn_world, entity->s.origin, end, vec3_origin, vec3_origin, MASK_PLAYERSOLID);
    VectorCopy(trace.endpos, entity->s.origin);
    DN_Link(entity);
    return 1;
}
