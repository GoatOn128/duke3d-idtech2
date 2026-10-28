/* Build visual units shared by the renderer and its behavioral checks. */
#ifndef DN_VISUAL_H
#define DN_VISUAL_H

#include <stdint.h>

static int DN_SpriteSectorShade(int encoded, unsigned flags, int ceilingstat, int floorshade,
                                int ceilingshade)
{
    int shade = encoded;
    if (flags & RF_DUKE_SECTOR_SHADE)
    {
        int sector = ceilingstat & 1 ? ceilingshade : floorshade;
        shade += sector < -127 ? -127 : sector;
    }
    return (int8_t)shade;
}

static void DN_SpriteOffsets(int width, int height, uint32_t animation, int rx, int ry, int cstat,
                             float *left, float *bottom)
{
    float xoffset = (int8_t)(animation >> 8) * rx / 64.f;
    float yoffset = (int8_t)(animation >> 16) * ry / 64.f;
    *left = -width * rx / 128.f + (cstat & 4 ? xoffset : -xoffset);
    /* Build screen Y points down; a positive ART Y offset raises the sprite. */
    *bottom = (cstat & (128 | 32) ? -height * ry / 128.f : 0) +
              ((cstat & 48) && (cstat & 8) ? -yoffset : yoffset);
}

static int DN_SectorFogRange(int visibility, int shade, float *start, float *end)
{
    int scale = visibility ? (uint8_t)(visibility + 16) : 16;
    if (!scale)
    {
        return 0; /* Visibility 240 deliberately disables distance shading. */
    }
    /* ENGINE.cpp drawsprite/getpalookup: default visibility 512 adds one shade
       per 1024 Build XY units, or 64 renderer units. GL linear fog approximates
       those palette steps, after TileImage has applied the surface's base shade. */
    float distance = 64.f * 16 / scale;
    *start = shade < 0 ? -shade * distance : 0;
    *end = (32 - shade) * distance;
    if (*end <= *start)
    {
        *end = *start + distance;
    }
    return 1;
}

#endif
