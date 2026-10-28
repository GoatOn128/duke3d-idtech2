#ifndef DN_SOUND_LOCAL_H
#define DN_SOUND_LOCAL_H
#include <AL/al.h>
#include <AL/alc.h>
#include <cstddef>
#include <cstdint>

struct dn_wave_t
{
    const uint8_t *pcm;
    size_t bytes;
    int rate, channels, width;
};
bool S_ParseWave(const void *data, size_t size, dn_wave_t *wave);
bool S_Ready(void);
#endif
