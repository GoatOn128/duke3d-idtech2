/* Original MUSICANDSFX emitters: radius ambience and randomized sector sounds. */
#include "../client/client.h"
#include "../cgame/cgame.h"
#include "dnambience.h"
#include <vector>

const dn_map_t *CM_DukeClientMap(void);
int CL_CGameSoundInfo(int id, dn_sound_t *sound);

static std::vector<uint32_t> next_sound;
static std::vector<uint8_t> entered_sectors;
static uint32_t random_seed = 17;

static uint32_t Random(void)
{
    random_seed = random_seed * 1664525u + 1013904223u;
    return random_seed;
}

static void SoundPath(const dn_sound_t &sound, char *path, size_t size)
{
    snprintf(path, size, "%s", sound.file);
    char *dot = strrchr(path, '.');
    if (dot)
    {
        snprintf(dot, size - (dot - path), ".wav");
    }
}

void S_ResetAmbience(void)
{
    next_sound.clear();
    entered_sectors.clear();
    random_seed = 17;
}

void S_UpdateAmbience(const float *origin)
{
    if (!Cvar_Get("s_ambience", "1", CVAR_ARCHIVE)->value)
    {
        return;
    }
    const dn_map_t *map = CM_DukeClientMap();
    if (!map || !map->numsectors)
    {
        return;
    }
    next_sound.resize(map->numsprites);
    entered_sectors.resize(map->numsectors);
    int listener_sector = DN_FindSectorXY(map, origin, -1);
    if (listener_sector >= 0)
    {
        entered_sectors[listener_sector] = 1;
    }
    uint32_t tick = cl.frame.playerstate.prediction.tick;
    for (int i = 0; i < map->numsprites; i++)
    {
        const dn_sprite_t &marker = map->sprites[i];
        if (marker.picnum != 5 || marker.sectnum < 0 || marker.sectnum >= map->numsectors ||
            marker.lotag < 0 || marker.lotag >= 999 ||
            (marker.pal == 1 && atoi(cl.configstrings[CS_MAXCLIENTS]) < 2))
        {
            continue;
        }
        const dn_sector_t &sector = map->sectors[marker.sectnum];
        int tag = (uint16_t)sector.lotag;
        /* Entry tags are consumed by checksectors on the server. Their static
           client-map copies must not suppress emitters in the entered room. */
        if (entered_sectors[marker.sectnum] && (tag == 32767 || (tag >= 10000 && tag < 16383)))
        {
            tag = 0;
        }
        if (tag >= 9 || sector.floorz == sector.ceilingz)
        {
            continue;
        }
        dn_sound_t sound;
        if (!CL_CGameSoundInfo(marker.lotag, &sound))
        {
            continue;
        }
        char path[MAX_QPATH];
        if (sound.flags & 2)
        {
            float dx = origin[0] - marker.x / 16.f;
            float dy = origin[1] + marker.y / 16.f;
            float dz = origin[2] + marker.z / 256.f;
            float distance = sqrtf(dx * dx + dy * dy + dz * dz);
            float radius = marker.hitag / 16.f;
            if (radius > 0 && distance < radius)
            {
                SoundPath(sound, path, sizeof(path));
                /* Native environment loops are mixed equally into both speakers. */
                S_AmbientLoop(i, path, 1 - distance / radius);
            }
        }
        if ((sound.flags & 16) && listener_sector == marker.sectnum && tick >= next_sound[i])
        {
            int id = marker.lotag + Random() % (uint32_t)(marker.hitag >= 0 ? marker.hitag + 1 : 1);
            if (CL_CGameSoundInfo(id, &sound))
            {
                SoundPath(sound, path, sizeof(path));
                S_StartLocalSound(path);
            }
            next_sound[i] = tick + 26 * 40 + Random() % (26 * 40);
        }
    }
}
