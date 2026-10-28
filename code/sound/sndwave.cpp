/* Bounded RIFF/PCM reader. Sound assets stay at their original sample rate. */
#include "sndlocal.h"
#include <cstring>
#include <climits>

static uint32_t Read32(const uint8_t *p)
{
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

static unsigned Read16(const uint8_t *p)
{
    return unsigned(p[0]) | unsigned(p[1]) << 8;
}

bool S_ParseWave(const void *data, size_t size, dn_wave_t *wave)
{
    *wave = {};
    const auto *p = static_cast<const uint8_t *>(data);
    if (!p || size < 12 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "WAVE", 4))
    {
        return false;
    }
    size_t end = size_t(Read32(p + 4)) + 8;
    if (end < 12 || end > size)
    {
        return false;
    }
    bool format = false;
    for (size_t offset = 12; offset < end;)
    {
        if (end - offset < 8)
        {
            return false;
        }
        size_t bytes = Read32(p + offset + 4);
        const uint8_t *chunk = p + offset + 8;
        if (bytes > end - offset - 8)
        {
            return false;
        }
        if (!memcmp(p + offset, "fmt ", 4))
        {
            if (bytes < 16 || Read16(chunk) != 1)
            {
                return false;
            }
            wave->channels = Read16(chunk + 2);
            uint32_t rate = Read32(chunk + 4);
            unsigned bits = Read16(chunk + 14);
            if ((wave->channels != 1 && wave->channels != 2) ||
                (bits != 8 && bits != 16) || !rate || rate > 192000 ||
                Read16(chunk + 12) != wave->channels * (bits / 8) ||
                Read32(chunk + 8) != rate * wave->channels * (bits / 8))
            {
                return false;
            }
            wave->rate = int(rate);
            wave->width = bits / 8;
            format = true;
        }
        else if (!memcmp(p + offset, "data", 4))
        {
            wave->pcm = chunk;
            wave->bytes = bytes;
        }
        offset += 8 + bytes;
        if (bytes & 1)
        {
            if (offset == end)
            {
                break; // Original assets may omit the final RIFF pad byte.
            }
            offset++;
        }
    }
    return format && wave->pcm && wave->bytes && wave->bytes <= INT_MAX &&
           wave->bytes % (wave->channels * wave->width) == 0;
}
