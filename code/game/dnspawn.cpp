/* Native Build map spawning and level transitions. */
#include "dnlocal.h"
#include "dncon.h"

static int changing_level;

edict_t *DN_MapSprite(int index)
{
    if (index < 0 || index >= dn_world.numsprites)
    {
        return NULL;
    }
    const dn_sprite_t *raw = &dn_world.sprites[index];
    vec3_t position = {raw->x / 16.f, -raw->y / 16.f, -raw->z / 256.f};
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        edict_t *entity = &dn_edicts[i];
        if (entity->inuse && entity->originaltile == raw->picnum &&
            VectorCompare(entity->spawnorigin, position))
        {
            return entity;
        }
    }
    return NULL;
}

void DN_ChangeLevel(int next)
{
    char text[96];
    if (changing_level)
    {
        return;
    }
    if (next)
    {
        DN_BeginIntermission(0);
        return;
    }
    changing_level = 1;
    /* gamemap archives destroyed props and moving sectors. Death needs a fresh map. */
    snprintf(text, sizeof(text), "map %s\n", dn_current_map);
    gi.AddCommandString(text);
}

void DN_SpawnEntities(char *mapname, char *entities, char *spawnpoint)
{
    char path[128], error[160];
    void *data;
    size_t size;
    changing_level = 0;
    for (int i = 0; i < globals.num_edicts; i++)
    {
        if (dn_edicts[i].inuse)
        {
            gi.unlinkentity(&dn_edicts[i]);
        }
    }
    memset(dn_edicts, 0, sizeof(edict_t) * MAX_EDICTS);
    for (int i = 1; i <= dn_maxclients; i++)
    {
        dn_edicts[i].client = &dn_clients[i - 1];
    }
    globals.num_edicts = dn_maxclients + 1;
    dn_edicts[0].inuse = true;
    dn_edicts[0].solid = SOLID_BSP;
    snprintf(dn_current_map, sizeof(dn_current_map), "%s", mapname);
    snprintf(path, sizeof(path), "maps/%s.map", mapname);
    DN_FreeMap(&dn_world);
    data = DN_ReadAsset(path, &size);
    if (!data)
    {
        gi.error("Missing Build map %s", path);
    }
    if (!DN_LoadMap(&dn_world, data, size, error, sizeof(error)))
    {
        gi.error("%s", error);
    }
    free(data);
    DN_ResetClock(0);
    DN_InitLevelResults();
    DN_InitMultiplayer();
    DN_SeedRandom(17);
    DN_ResetSounds();
    dn_sprite_model = gi.modelindex("sprites/duke.sp2");
    gi.configstring(CS_NAME, mapname);
    {
        int episode = 0, level = 0;
        char track[MAX_QPATH];
        const char *name = "";
        if (sscanf(mapname, "e%dl%d", &episode, &level) == 2)
        {
            name = DN_ConMusic(episode, level);
        }
        snprintf(track, sizeof(track), "%s", name);
        gi.configstring(CS_CDTRACK, track);
    }
    gi.configstring(CS_SKY, "");
    gi.configstring(CS_SKYAXIS, "0 0 1");
    gi.configstring(CS_SKYROTATE, "0");
    gi.configstring(CS_STATUSBAR, "");
    {
        char limits[160] = {};
        static const char *names[] = {"MAXPISTOLAMMO",     "MAXSHOTGUNAMMO",  "MAXCHAINGUNAMMO",
                                      "MAXRPGAMMO",        "MAXHANDBOMBAMMO", "MAXSHRINKERAMMO",
                                      "MAXDEVISTATORAMMO", "MAXTRIPBOMBAMMO", "MAXFREEZEAMMO",
                                      "MAXGROWAMMO"};
        size_t used = 0;
        for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        {
            used +=
                snprintf(limits + used, sizeof(limits) - used, "%d ", DN_ConSetting(names[i], 200));
        }
        gi.configstring(CS_GENERAL, limits);
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        dn_sprite_t *s = &dn_world.sprites[i];
        edict_t *e;
        if (s->picnum < 11 || s->picnum == DN_ConSymbol("APLAYER", 1405) || !s->xrepeat ||
            !s->yrepeat || !DN_AllowMapSprite(s))
        {
            continue;
        }
        /* Build sprite lotag selects the minimum skill for enemies. */
        if (DN_IsEnemy(s->picnum) && s->lotag > gi.cvar("skill", "1", 0)->value + 1)
        {
            continue;
        }
        e = DN_NewEntity();
        e->tile = e->originaltile = s->picnum;
        e->cstat = s->cstat;
        e->xrepeat = s->xrepeat;
        e->yrepeat = s->yrepeat;
        e->pal = s->pal;
        e->shade = s->shade;
        e->sector = s->sectnum;
        e->clipdist = s->clipdist;
        e->lotag = s->lotag;
        e->hitag = s->hitag;
        e->health = s->extra;
        e->spawnedby = s->picnum;
        e->enemy = DN_IsEnemy(s->picnum);
        VectorSet(e->s.origin, s->x / 16.f, -s->y / 16.f, -s->z / 256.f);
        VectorCopy(e->s.origin, e->spawnorigin);
        e->s.angles[1] = -s->ang * 360.f / 2048;
        DN_InitMapSwitch(e);
        if (DN_InitFaller(e))
        {
            DN_Link(e);
            continue;
        }
        DN_ConInitActor(e);
        DN_InitEnemy(e, true);
        DN_InitEffect(e, NULL);
        DN_InitPickup(e, NULL);
        DN_InitDecal(e, NULL);
        DN_InitDestructible(e);
        DN_InitVehicle(e);
        DN_Link(e);
    }
    DN_InitSectors();
    DN_InitInteractions();
    gi.dprintf("Duke game spawned %d entities on %s\n", globals.num_edicts, mapname);
}
