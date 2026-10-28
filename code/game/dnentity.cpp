/* Entity allocation, sprite state, tile metadata and linking. */
#include "dnlocal.h"
#include "dncon.h"
#include "../qcommon/dnmath.h"
#include "../qcommon/dnvisual.h"

static struct
{
    int16_t width, height;
    uint32_t anim;
} tileinfo[DN_MAX_TILES];

void DN_SpriteSize(const edict_t *entity, float *width, float *height)
{
    int tile = entity->s.frame;
    *width =
        (tile >= 0 && tile < DN_MAX_TILES ? tileinfo[tile].width : 32) * entity->xrepeat / 64.f;
    *height =
        (tile >= 0 && tile < DN_MAX_TILES ? tileinfo[tile].height : 40) * entity->yrepeat / 64.f;
}

float DN_ActorHeight(const edict_t *entity)
{
    return (entity->tile >= 0 && entity->tile < DN_MAX_TILES ? tileinfo[entity->tile].height : 40) *
           entity->yrepeat / 64.f;
}

edict_t *DN_NewEntity(void)
{
    int i;
    for (i = dn_maxclients + 1; i < globals.max_edicts; i++)
    {
        if (!dn_edicts[i].inuse)
        {
            edict_t *e = &dn_edicts[i];
            uint32_t generation = e->s.effects + 1;
            memset(e, 0, sizeof(*e));
            e->s.effects = generation;
            e->s.renderfx = RF_DUKE_SPRITE;
            e->inuse = true;
            e->s.number = i;
            e->actor = -1;
            e->stayput = -1;
            e->zrange_clock = -1;
            e->clipdist = 16;
            if (i >= globals.num_edicts)
            {
                globals.num_edicts = i + 1;
            }
            return e;
        }
    }
    gi.error("Duke entity limit exceeded");
    return NULL;
}

void DN_Remove(edict_t *e)
{
    if (e->client)
    {
        return;
    }
    gi.unlinkentity(e);
    e->inuse = false;
    e->s.modelindex = 0;
    e->solid = SOLID_NOT;
}

void DN_Link(edict_t *e)
{
    int tile = e->tile;
    float width, height;
    if (!e->inuse)
    {
        return;
    }
    if (e->s.renderfx & (RF_DUKE_WORLD | RF_BEAM))
    {
        gi.linkentity(e);
        return;
    }
    /* Build setsprite/updatesector uses XY. Losing the sector on a one-tick
       floor overshoot prevents fall and CON floor queries from ever recovering. */
    /* Wall switches may sit exactly on a polygon edge (E2L1 sprite 252).
       Build retains their authored sector; a point-in-polygon update can
       otherwise orphan the switch and remove it from every use query.
       A used access panel keeps that sector after changing to its spent tile. */
    if (!(e->cstat & 16) ||
        (!DN_IsWallSwitch(e->tile) && !DN_IsWallSwitch(e->originaltile) &&
         !DN_IsBathroomFixture(e->tile) && e->originaltile != DN_ConSymbol("NUKEBUTTON", 142)) ||
        e->sector < 0 || e->sector >= dn_world.numsectors)
    {
        e->sector = DN_FindSectorXY(&dn_world, e->s.origin, e->sector);
    }
    e->s.modelindex = (e->cstat & 32768) ? 0 : dn_sprite_model;
    if (e->action > 0 && e->action < 2048)
    {
        dn_action_t *a = &dn_actions[e->action];
        tile += a->value[0] + e->animframe * a->value[2];
        e->viewtype = a->value[2];
    }
    else
    {
        e->viewtype = 1;
        if (e->tile == DN_ConSymbol("RECON", 1960))
        {
            e->viewtype = 7;
            if (abs(e->native_state[3]) > 64)
            {
                tile += 7;
            }
        }
    }
    e->s.frame = tile + (e->fragment ? e->animframe : 0);
    e->s.animation = (dn_animation_t){0};
    if (e->action > 0 && e->action < 2048 && !e->fragment && !e->projectile && e->actor >= 0)
    {
        e->s.animation.tick = (uint32_t)lroundf(dn_time * DN_SIMULATION_HZ);
        e->s.animation.tile = e->tile;
        e->s.animation.action = e->action;
        e->s.animation.frame = e->animframe;
        e->s.animation.clock = e->animclock;
        e->s.animation.flags = e->dormant ? 0 : DN_ANIMATION_RUNNING;
    }
    e->s.skinnum =
        e->xrepeat | (e->yrepeat << 8) | ((e->cstat & 0xc3ff) << 16) | ((e->viewtype & 15) << 26);
    int shade;
    int lighting = DN_SpriteLighting(e, &shade);
    /* One byte carries all signed shade values exactly, unlike shade-as-degrees. */
    e->s.angles[0] = (uint8_t)(shade + 128) * 360.f / 256;
    e->s.angles[2] = e->pal * 360.f / 256;
    e->s.renderfx = RF_DUKE_SPRITE | lighting | (e->cstat & 2 ? RF_TRANSLUCENT : 0);
    if (e->client)
    {
        e->s.renderfx |= RF_DUKE_OWN_PALETTE;
    }
    /* Action frames change the drawing, not the actor's collision dimensions. */
    width =
        (e->tile >= 0 && e->tile < DN_MAX_TILES ? tileinfo[e->tile].width : 32) * e->xrepeat / 64.f;
    height = (e->tile >= 0 && e->tile < DN_MAX_TILES ? tileinfo[e->tile].height : 40) * e->yrepeat /
             64.f;
    if (!e->client)
    {
        float radius = fmaxf(1, fminf(width * .4f, e->clipdist / 4.f));
        if (e->enemy)
        {
            const dn_actor_t *actor = DN_ConActor(e->tile);
            radius = e->xrepeat > 60                           ? 64
                     : e->tile == DN_ConSymbol("LIZMAN", 2120) ? 292.f / 16
                     : actor && actor->enemy                   ? e->clipdist / 4.f
                                                               : 12;
        }
        VectorSet(e->mins, -radius, -radius, e->cstat & 128 ? -height / 2 : 0);
        VectorSet(e->maxs, radius, radius, e->cstat & 128 ? height / 2 : height);
        if ((e->cstat & 48) == 32)
        {
            float left, bottom;
            int valid = e->tile >= 0 && e->tile < DN_MAX_TILES;
            DN_SpriteOffsets(valid ? tileinfo[e->tile].width : 32,
                             valid ? tileinfo[e->tile].height : 40,
                             valid ? tileinfo[e->tile].anim : 0, e->xrepeat, e->yrepeat, e->cstat,
                             &left, &bottom);
            VectorSet(e->mins, left, bottom, 0);
            VectorSet(e->maxs, left + width, bottom + height, 0);
        }
        e->solid = (e->cstat & (1 | 256)) ? SOLID_BBOX : SOLID_NOT;
        e->svflags = e->enemy ? SVF_MONSTER : 0;
    }
    gi.linkentity(e);
}

edict_t *DN_SpawnSprite(edict_t *parent, int tile)
{
    edict_t *e = DN_NewEntity();
    if (tile < 0 || tile >= DN_MAX_TILES || (DN_IsDeathmatch() && DN_IsEnemy(tile)))
    {
        DN_Remove(e);
        return e;
    }
    e->tile = e->originaltile = tile;
    e->xrepeat = e->yrepeat = 32;
    e->spawnedby = parent ? parent->tile : tile;
    if (parent)
    {
        VectorCopy(parent->s.origin, e->s.origin);
        e->sector = parent->sector;
        e->s.angles[1] = parent->s.angles[1];
        e->owner = parent;
    }
    e->enemy = DN_IsEnemy(tile);
    e->cstat = e->enemy ? 257 : 0;
    DN_ConInitActor(e);
    DN_InitEnemy(e, false);
    DN_InitEffect(e, parent);
    DN_InitPickup(e, parent);
    DN_InitDecal(e, parent);
    DN_InitGib(e);
    DN_InitDestructible(e);
    DN_Link(e);
    return e;
}

edict_t *DN_NearestPlayer(edict_t *e, float *distance)
{
    edict_t *best = NULL;
    float bestd = 1e30f;
    for (int i = 1; i <= dn_maxclients; i++)
    {
        if (dn_edicts[i].inuse && dn_clients[i - 1].connected)
        {
            edict_t *player = &dn_edicts[i];
            /* findplayer() uses Manhattan XY plus a scaled eye-to-feet Z term. */
            float eye = player->s.origin[2] + player->client->ps.viewoffset[2];
            float d = fabsf(player->s.origin[0] - e->s.origin[0]) +
                      fabsf(player->s.origin[1] - e->s.origin[1]) +
                      fabsf(e->s.origin[2] - eye + 28);
            if (d < bestd)
            {
                bestd = d;
                best = &dn_edicts[i];
            }
        }
    }
    if (distance)
    {
        *distance = bestd;
    }
    return best;
}

void DN_InitTiles(void)
{
    DN_InitLighting();
    size_t size;
    void *data = DN_ReadAsset("sprites/tiles.dat", &size);
    if (!data || size != sizeof(tileinfo))
    {
        free(data);
        gi.error("Missing Duke tiles: run prepare_assets.py");
    }
    memcpy(tileinfo, data, size);
    free(data);
    data = DN_ReadAsset("sprites/tables.dat", &size);
    if (!DN_MathInit(data, size))
    {
        free(data);
        gi.error("Missing Build TABLES.DAT: run prepare_assets.py");
    }
    free(data);
}
