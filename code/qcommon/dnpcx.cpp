#include "dnpcx.h"
#include <stdlib.h>
#include <string.h>

static unsigned Read16(const unsigned char *data)
{
    return data[0] | ((unsigned)data[1] << 8);
}

int DN_DecodePCX(const void *buffer, size_t size, unsigned char **pixels, int *width, int *height,
                 unsigned char *palette)
{
    const unsigned char *data = (const unsigned char *)(buffer);
    size_t position = 128, end;
    *pixels = NULL;
    *width = *height = 0;
    if (!data || size < 128 + 769 || data[0] != 10 || data[1] != 5 || data[2] != 1 ||
        data[3] != 8 || data[65] != 1 || data[size - 769] != 12)
    {
        return 0;
    }
    int w = (int)Read16(data + 8) - (int)Read16(data + 4) + 1;
    int h = (int)Read16(data + 10) - (int)Read16(data + 6) + 1;
    unsigned stride = Read16(data + 66);
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096 || stride < (unsigned)w || stride > 8192)
    {
        return 0;
    }
    unsigned char *output = (unsigned char *)(malloc((size_t)w * h));
    if (!output)
    {
        return 0;
    }
    end = size - 769;
    for (int y = 0; y < h; y++)
    {
        unsigned x = 0;
        while (x < stride)
        {
            unsigned run = 1, value;
            if (position >= end)
            {
                goto malformed;
            }
            value = data[position++];
            if ((value & 192) == 192)
            {
                run = value & 63;
                if (!run || position >= end)
                {
                    goto malformed;
                }
                value = data[position++];
            }
            if (run > stride - x)
            {
                goto malformed;
            }
            for (unsigned i = 0; i < run; i++, x++)
            {
                if (x < (unsigned)w)
                {
                    output[(size_t)y * w + x] = value;
                }
            }
        }
    }
    if (palette)
    {
        memcpy(palette, data + size - 768, 768);
    }
    *pixels = output;
    *width = w;
    *height = h;
    return 1;
malformed:
    free(output);
    return 0;
}
