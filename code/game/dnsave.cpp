/* Versioned saves contain values and edict indices, never DLL addresses. */
#include "dnlocal.h"
#include "dncon.h"

#define DN_SAVE_VERSION 24

typedef struct
{
    char magic[8];
    uint32_t version, count, extrasize;
    float time;
    uint32_t program;
} save_header_t;

typedef struct
{
    char map[MAX_QPATH];
    uint32_t sectors, walls, sprites, random_state;
} save_world_t;

typedef struct
{
    entity_state_t state;
    int32_t inuse, solid, svflags;
    vec3_t mins, maxs;
    int32_t owner, clipmask;
} save_entity_t;

void DN_WriteSaveData(FILE *file, const void *data, size_t size, size_t count)
{
    if (count && fwrite(data, size, count, file) != count)
    {
        fclose(file);
        gi.error("Failed writing Duke save");
    }
}

void DN_ReadSaveData(FILE *file, void *data, size_t size, size_t count)
{
    if (count && fread(data, size, count, file) != count)
    {
        fclose(file);
        gi.error("Truncated Duke save");
    }
}

static void CloseSave(FILE *file)
{
    if (fclose(file))
    {
        gi.error("Failed closing Duke save");
    }
}

static void InvalidSave(FILE *file, const char *message)
{
    fclose(file);
    gi.error("%s", message);
}

void DN_WriteGame(char *filename, qboolean autosave)
{
    FILE *file = fopen(filename, "wb");
    save_header_t header = {"DNGAME", DN_SAVE_VERSION, static_cast<uint32_t>(dn_maxclients), sizeof(gclient_t), 0};
    header.program = DN_ConChecksum();
    if (!file)
    {
        gi.error("Cannot save %s", filename);
    }
    DN_WriteSaveData(file, &header, sizeof(header), 1);
    DN_WriteSaveData(file, dn_clients, sizeof(*dn_clients), dn_maxclients);
    CloseSave(file);
}

void DN_ReadGame(char *filename)
{
    FILE *file = fopen(filename, "rb");
    save_header_t header;
    if (!file)
    {
        gi.error("Cannot read %s", filename);
    }
    DN_ReadSaveData(file, &header, sizeof(header), 1);
    if (memcmp(header.magic, "DNGAME", 7) || header.version != DN_SAVE_VERSION ||
        header.count != dn_maxclients || header.extrasize != sizeof(gclient_t) ||
        header.program != DN_ConChecksum())
    {
        InvalidSave(file, "Incompatible Duke game save");
    }
    DN_ReadSaveData(file, dn_clients, sizeof(*dn_clients), dn_maxclients);
    for (int i = 0; i < dn_maxclients; i++)
    {
        if (dn_clients[i].weapon < 0 || dn_clients[i].weapon >= DN_MAX_WEAPONS)
        {
            InvalidSave(file, "Invalid saved weapon");
        }
        if (dn_clients[i].transport_hold < 0 || dn_clients[i].transport_hold > 58 ||
            dn_clients[i].jetpack_on < 0 || dn_clients[i].jetpack_on > 255)
        {
            InvalidSave(file, "Invalid saved player transport state");
        }
        if (dn_clients[i].access_ticks < 0 || dn_clients[i].access_ticks > 20 ||
            dn_clients[i].access_wall < -1 || dn_clients[i].access_wall >= DN_MAX_WALLS ||
            dn_clients[i].access_sprite < -1 || dn_clients[i].access_sprite >= MAX_EDICTS)
        {
            InvalidSave(file, "Invalid saved access operation");
        }
        if (dn_clients[i].weapon_frame < 0 || dn_clients[i].weapon_frame > 31 ||
            dn_clients[i].weapon_side < 0 || dn_clients[i].weapon_side > 1 ||
            dn_clients[i].weapon_position < -9 || dn_clients[i].weapon_position > 10 ||
            dn_clients[i].weapon_sway < 0 || dn_clients[i].weapon_sway > 4095 ||
            dn_clients[i].weapon_hold < 0 || dn_clients[i].weapon_hold > 64)
        {
            InvalidSave(file, "Invalid saved weapon sequence");
        }
        if (dn_clients[i].last_pissed_time < 0 ||
            dn_clients[i].last_pissed_time > DN_TOILET_COOLDOWN)
        {
            InvalidSave(file, "Invalid saved toilet cooldown");
        }
        dn_clients[i].name[sizeof(dn_clients[i].name) - 1] = 0;
        if (dn_clients[i].secrets < 0 || dn_clients[i].secrets > DN_MAX_SECTORS)
        {
            InvalidSave(file, "Invalid saved secret count");
        }
    }
    CloseSave(file);
}

void DN_WriteLevel(char *filename)
{
    FILE *file = fopen(filename, "wb");
    save_header_t header = {"DNLEVEL", DN_SAVE_VERSION, static_cast<uint32_t>(globals.num_edicts),
                            sizeof(edict_t) - offsetof(edict_t, tile), dn_time};
    save_world_t world = {};
    header.program = DN_ConChecksum();
    if (!file)
    {
        gi.error("Cannot save %s", filename);
    }
    snprintf(world.map, sizeof(world.map), "%s", dn_current_map);
    world.sectors = dn_world.numsectors;
    world.walls = dn_world.numwalls;
    world.sprites = dn_world.numsprites;
    world.random_state = DN_RandomState();
    DN_WriteSaveData(file, &header, sizeof(header), 1);
    DN_WriteSaveData(file, &world, sizeof(world), 1);
    for (int i = 0; i < globals.num_edicts; i++)
    {
        const edict_t *entity = &dn_edicts[i];
        save_entity_t saved = {};
        saved.state = entity->s;
        saved.inuse = entity->inuse;
        saved.solid = entity->solid;
        saved.svflags = entity->svflags;
        VectorCopy(entity->mins, saved.mins);
        VectorCopy(entity->maxs, saved.maxs);
        saved.owner = entity->owner ? (int)(entity->owner - dn_edicts) : -1;
        saved.clipmask = entity->clipmask;
        DN_WriteSaveData(file, &saved, sizeof(saved), 1);
        DN_WriteSaveData(file, &entity->tile, header.extrasize, 1);
    }
    DN_WriteSaveData(file, dn_world.sectors, sizeof(*dn_world.sectors), dn_world.numsectors);
    DN_WriteSaveData(file, dn_world.walls, sizeof(*dn_world.walls), dn_world.numwalls);
    DN_WriteSaveData(file, dn_world.sprites, sizeof(*dn_world.sprites), dn_world.numsprites);
    DN_WriteSectorState(file);
    DN_WriteLevelResults(file);
    CloseSave(file);
}

static void ValidateWorld(FILE *file)
{
    for (int i = 0; i < dn_world.numsectors; i++)
    {
        const dn_sector_t *sector = &dn_world.sectors[i];
        if (sector->wallptr < 0 || sector->wallnum < 2 ||
            sector->wallptr + sector->wallnum > dn_world.numwalls)
        {
            InvalidSave(file, "Invalid saved sector wall range");
        }
        for (int j = sector->wallptr; j < sector->wallptr + sector->wallnum; j++)
        {
            const dn_wall_t *wall = &dn_world.walls[j];
            if (wall->point2 < sector->wallptr ||
                wall->point2 >= sector->wallptr + sector->wallnum || wall->nextwall < -1 ||
                wall->nextwall >= dn_world.numwalls || wall->nextsector < -1 ||
                wall->nextsector >= dn_world.numsectors)
            {
                InvalidSave(file, "Invalid saved wall link");
            }
        }
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        if (dn_world.sprites[i].sectnum < 0 || dn_world.sprites[i].sectnum >= dn_world.numsectors)
        {
            InvalidSave(file, "Invalid saved sprite sector");
        }
    }
}

void DN_ReadLevel(char *filename)
{
    FILE *file = fopen(filename, "rb");
    save_header_t header;
    save_world_t world;
    if (!file)
    {
        gi.error("Cannot read %s", filename);
    }
    DN_ReadSaveData(file, &header, sizeof(header), 1);
    DN_ReadSaveData(file, &world, sizeof(world), 1);
    if (memcmp(header.magic, "DNLEVEL", 8) || header.version != DN_SAVE_VERSION ||
        header.count < (uint32_t)dn_maxclients + 1 || header.count > MAX_EDICTS ||
        header.extrasize != sizeof(edict_t) - offsetof(edict_t, tile) ||
        header.program != DN_ConChecksum() || !isfinite(header.time) || header.time < 0 ||
        (double)header.time > INT32_MAX / 10.0 ||
        (!memchr(world.map, 0, sizeof(world.map)) || strcmp(world.map, dn_current_map)) ||
        world.sectors != dn_world.numsectors || world.walls != dn_world.numwalls ||
        world.sprites != dn_world.numsprites)
    {
        InvalidSave(file, "Incompatible Duke level save");
    }
    for (int i = 0; i < globals.num_edicts; i++)
    {
        gi.unlinkentity(&dn_edicts[i]);
    }
    memset(dn_edicts, 0, MAX_EDICTS * sizeof(*dn_edicts));
    globals.num_edicts = header.count;
    DN_ResetClock(header.time);
    DN_SeedRandom(world.random_state);
    for (int i = 0; i < globals.num_edicts; i++)
    {
        edict_t *entity = &dn_edicts[i];
        save_entity_t saved;
        DN_ReadSaveData(file, &saved, sizeof(saved), 1);
        DN_ReadSaveData(file, &entity->tile, header.extrasize, 1);
        if ((saved.inuse && saved.state.number != i) || saved.owner < -1 ||
            saved.owner >= (int)header.count || entity->sector < -1 ||
            entity->sector >= dn_world.numsectors || entity->tile < 0 ||
            entity->tile >= DN_MAX_TILES || entity->faller < 0 || entity->faller > 2 ||
            entity->zrange_clock < -1 || entity->zrange_clock > 6 ||
            !isfinite(entity->cached_floor) || !isfinite(entity->cached_ceiling))
        {
            InvalidSave(file, "Invalid saved actor");
        }
        entity->s = saved.state;
        entity->inuse = saved.inuse;
        /* The saved old_origin is already initialized. Relinking must not
           replace it with the restored current position. */
        entity->linkcount = saved.inuse ? 1 : 0;
        entity->solid = (solid_t)(saved.solid);
        entity->svflags = saved.svflags;
        entity->clipmask = saved.clipmask;
        entity->owner = saved.owner >= 0 ? &dn_edicts[saved.owner] : NULL;
        VectorCopy(saved.mins, entity->mins);
        VectorCopy(saved.maxs, entity->maxs);
        if (i > 0 && i <= dn_maxclients)
        {
            entity->client = &dn_clients[i - 1];
            entity->client->restore_player = entity->inuse;
        }
    }
    DN_ReadSaveData(file, dn_world.sectors, sizeof(*dn_world.sectors), dn_world.numsectors);
    DN_ReadSaveData(file, dn_world.walls, sizeof(*dn_world.walls), dn_world.numwalls);
    DN_ReadSaveData(file, dn_world.sprites, sizeof(*dn_world.sprites), dn_world.numsprites);
    ValidateWorld(file);
    DN_ReadSectorState(file);
    DN_ReadLevelResults(file);
    CloseSave(file);
    DN_RestoreSectorMarkers();
    /* Publish restored geometry before actors are linked against it. */
    for (int i = 1; i < globals.num_edicts; i++)
    {
        if (dn_edicts[i].inuse && (dn_edicts[i].s.renderfx & RF_DUKE_WORLD))
        {
            DN_Link(&dn_edicts[i]);
        }
    }
    for (int i = 1; i < globals.num_edicts; i++)
    {
        if (dn_edicts[i].inuse && !(dn_edicts[i].s.renderfx & RF_DUKE_WORLD))
        {
            DN_Link(&dn_edicts[i]);
        }
    }
}
