/* Game sound registration and actor sound commands. */
#include "dnlocal.h"
#include "dncon.h"
#include <ctype.h>

static int sound_indices[1024];
static float sound_durations[1024];

static uint32_t ReadU32(const byte *data)
{
    uint32_t value;
    memcpy(&value, data, sizeof(value));
    return value;
}

static float SoundDuration(const char *name)
{
    char path[MAX_QPATH + 8];
    size_t length;
    byte *data;
    uint32_t bytes_per_second = 0, samples = 0;
    float duration = .5f;
    snprintf(path, sizeof(path), "sound/%s", name);
    data = (byte *)(DN_ReadAsset(path, &length));
    if (data && length >= 12 && !memcmp(data, "RIFF", 4) && !memcmp(data + 8, "WAVE", 4))
    {
        size_t offset = 12;
        while (length - offset >= 8)
        {
            uint32_t size = ReadU32(data + offset + 4);
            if (size > length - offset - 8)
            {
                break;
            }
            if (!memcmp(data + offset, "fmt ", 4) && size >= 16)
            {
                bytes_per_second = ReadU32(data + offset + 8 + 8);
            }
            if (!memcmp(data + offset, "data", 4))
            {
                samples = size;
            }
            offset += 8 + (size_t)size;
            if (size & 1)
            {
                if (offset == length)
                {
                    break;
                }
                offset++;
            }
        }
        if (bytes_per_second && samples)
        {
            duration = samples / (float)bytes_per_second;
        }
    }
    free(data);
    return duration;
}

void DN_PlaySound(edict_t *ent, int id, int once, int global)
{
    char path[96];
    char *dot;
    if (!ent || id < 0 || id >= 1024 || !dn_sounds[id].file[0])
    {
        return;
    }
    if (once && ent->sound_id == id + 1 && ent->sound_end > dn_time)
    {
        return;
    }
    snprintf(path, sizeof(path), "%s", dn_sounds[id].file);
    for (int i = 0; path[i]; i++)
    {
        path[i] = (char)tolower((unsigned char)path[i]);
    }
    dot = strrchr(path, '.');
    if (dot)
    {
        snprintf(dot, sizeof(path) - (dot - path), ".wav");
    }
    if (!sound_indices[id])
    {
        sound_indices[id] = gi.soundindex(path);
        sound_durations[id] = SoundDuration(path);
    }
    /* Native flag 16 makes distant war/vehicle sounds audible at full volume. */
    int full_volume = global || (dn_sounds[id].flags & (16 | 128));
    gi.sound(ent, CHAN_AUTO, sound_indices[id], 1, full_volume ? ATTN_NONE : ATTN_NORM, 0);
    ent->sound_id = id + 1;
    ent->sound_end = dn_time + sound_durations[id];
}

void DN_StopSound(edict_t *ent, int id)
{
    ent->sound_id = 0;
    gi.sound(ent, CHAN_AUTO | CHAN_RELIABLE, 0, 0, ATTN_NORM, 0);
}

int DN_SoundPlaying(edict_t *ent)
{
    return ent->sound_id && ent->sound_end > dn_time;
}

void DN_ResetSounds(void)
{
    memset(sound_indices, 0, sizeof(sound_indices));
    memset(sound_durations, 0, sizeof(sound_durations));
}
