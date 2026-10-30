/* Converts 3drealms/duke3d.grp into the basedn/ assets the engine loads at
   runtime. Every output format matches the loader that reads it in this
   repository; comments name that loader wherever the format is not obvious.
   Written from scratch for this project; no third-party code is included. */

#include <ctype.h>
#include <direct.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TILES 6144 /* qcommon/dnmap.h DN_MAX_TILES */
#define SCREEN_W 320
#define SCREEN_H 200

#define LOGO_FRAMES 26      /* 75 ms each; bombexpl plays at frame 19 */
#define LOGO_FADE_FRAMES 10

#define PATH_LEN 1024

static void fail(const char *message)
{
    fprintf(stderr, "convertgrp: %s\n", message);
    exit(1);
}

static unsigned rd16(const uint8_t *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned long rd32(const uint8_t *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static void put16(uint8_t *p, unsigned v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
}

static void put32(uint8_t *p, unsigned long v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

/* ---------------------------------------------------------------------------
   GRP container
   --------------------------------------------------------------------------- */

/* GRP layout: 12-byte "KenSilverman" magic, int32 file count, then 16-byte
   entries (12-byte name, int32 size). File data follows the entry table in
   entry order. */

static uint8_t *grp;
static long grp_size;
static int file_count;
static char (*file_name)[13];
static long *file_offset;
static int *file_size;

static void load_grp(const char *path)
{
    FILE *f = fopen(path, "rb");
    int i;
    long offset;

    if (!f)
    {
        fail("cannot open 3drealms/duke3d.grp");
    }
    fseek(f, 0, SEEK_END);
    grp_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    grp = (uint8_t *)malloc((size_t)grp_size);
    if (!grp || fread(grp, 1, (size_t)grp_size, f) != (size_t)grp_size)
    {
        fail("cannot read 3drealms/duke3d.grp");
    }
    fclose(f);

    if (grp_size < 16 || memcmp(grp, "KenSilverman", 12) != 0)
    {
        fail("3drealms/duke3d.grp is not a KenSilverman group file");
    }
    file_count = (int)rd32(grp + 12);
    file_name = (char (*)[13])malloc((size_t)file_count * 13);
    file_offset = (long *)malloc((size_t)file_count * sizeof(long));
    file_size = (int *)malloc((size_t)file_count * sizeof(int));
    if (!file_name || !file_offset || !file_size)
    {
        fail("out of memory");
    }
    offset = 16 + (long)file_count * 16;
    for (i = 0; i < file_count; i++)
    {
        int base = 16 + i * 16, j;
        for (j = 0; j < 12 && grp[base + j]; j++)
        {
            file_name[i][j] = (char)tolower(grp[base + j]);
        }
        file_name[i][j] = '\0';
        file_size[i] = (int)rd32(grp + base + 12);
        file_offset[i] = offset;
        offset += file_size[i];
    }
}

static int find_file(const char *name)
{
    int i;
    for (i = 0; i < file_count; i++)
    {
        if (!strcmp(file_name[i], name))
        {
            return i;
        }
    }
    return -1;
}

static const uint8_t *grp_data(const char *name, int *size_out)
{
    int i = find_file(name);
    if (i < 0)
    {
        return NULL;
    }
    *size_out = file_size[i];
    return grp + file_offset[i];
}

static int ends_with(const char *name, const char *suffix)
{
    size_t n = strlen(name), s = strlen(suffix);
    return n >= s && !strcmp(name + n - s, suffix);
}

/* ---------------------------------------------------------------------------
   Disk helpers
   --------------------------------------------------------------------------- */

static void write_file(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f || (size && fwrite(data, 1, size, f) != size))
    {
        fail("cannot write output file");
    }
    fclose(f);
}

/* Creates every path component; forward slashes work in the Windows CRT. */
static void make_dirs(const char *path)
{
    char buffer[PATH_LEN];
    char *p;
    strncpy(buffer, path, PATH_LEN - 1);
    buffer[PATH_LEN - 1] = '\0';
    for (p = buffer; *p; p++)
    {
        if (*p == '/' && p > buffer)
        {
            *p = '\0';
            _mkdir(buffer);
            *p = '/';
        }
    }
    _mkdir(buffer);
}

/* Removes files the previous run left in a directory the tool owns. */
static void clear_generated(const char *dir)
{
    char pattern[PATH_LEN], path[PATH_LEN];
    struct _finddata_t info;
    intptr_t handle;

    snprintf(pattern, sizeof(pattern), "%s/*", dir);
    handle = _findfirst(pattern, &info);
    if (handle == -1)
    {
        return;
    }
    do
    {
        if (!(info.attrib & _A_SUBDIR))
        {
            snprintf(path, sizeof(path), "%s/%s", dir, info.name);
            remove(path);
        }
    } while (_findnext(handle, &info) == 0);
    _findclose(handle);
}

/* ---------------------------------------------------------------------------
   ART tiles, palette and lookup tables
   --------------------------------------------------------------------------- */

static uint16_t tile_w[MAX_TILES], tile_h[MAX_TILES];
static uint32_t tile_anim[MAX_TILES];
static uint8_t *tile_pix[MAX_TILES]; /* row-major pixels, NULL when absent */

static uint8_t palette24[768];
static uint8_t shade_tab[32][256];
static uint8_t remap_tab[256][256]; /* pal number -> remap, identity by default */

/* ART layout: version, unused tile count, first and last tile, then width,
   height and picanm arrays for that range, then the pixel data. Pixels are
   column-major: consecutive bytes fill one column from the top before
   moving right. */

static void load_art(void)
{
    int i, j;

    for (i = 0; i < file_count; i++)
    {
        const uint8_t *data = grp + file_offset[i];
        int size = file_size[i];
        int start, end, count, pos;
        uint16_t *widths, *heights;

        if (!ends_with(file_name[i], ".art") || size < 16)
        {
            continue;
        }
        start = (int)rd32(data + 8);
        end = (int)rd32(data + 12);
        if (start < 0 || end < start || end >= MAX_TILES)
        {
            fail("ART tile range outside 0..6143");
        }
        count = end - start + 1;
        if (size < 16 + count * 8)
        {
            fail("ART file truncated before tile metadata");
        }
        widths = (uint16_t *)malloc((size_t)count * 2);
        heights = (uint16_t *)malloc((size_t)count * 2);
        if (!widths || !heights)
        {
            fail("out of memory");
        }
        pos = 16;
        for (j = 0; j < count; j++)
        {
            widths[j] = (uint16_t)rd16(data + pos + j * 2);
        }
        pos += count * 2;
        for (j = 0; j < count; j++)
        {
            heights[j] = (uint16_t)rd16(data + pos + j * 2);
        }
        pos += count * 2;
        for (j = 0; j < count; j++)
        {
            int tile = start + j;
            tile_w[tile] = widths[j];
            tile_h[tile] = heights[j];
            tile_anim[tile] = (uint32_t)rd32(data + pos + j * 4);
        }
        pos += count * 4;
        for (j = 0; j < count; j++)
        {
            int tile = start + j;
            int w = widths[j], h = heights[j];
            int x, y;
            uint8_t *pixels;

            if (pos + w * h > size)
            {
                fail("ART file truncated before tile pixels");
            }
            pixels = (uint8_t *)malloc((size_t)(w * h ? w * h : 1));
            if (!pixels)
            {
                fail("out of memory");
            }
            for (x = 0; x < w; x++)
            {
                for (y = 0; y < h; y++)
                {
                    pixels[y * w + x] = data[pos + x * h + y];
                }
            }
            tile_pix[tile] = pixels;
            pos += w * h;
        }
        free(widths);
        free(heights);
    }
}

/* PALETTE.DAT: 768 VGA 0..63 bytes (times four for 24-bit), then int16
   numshades and numshades*256 shade tables. The renderer reads a fixed
   32*256 block (dnrender.cpp LoadPalettes). */

static void load_palette(void)
{
    const uint8_t *data;
    int size, i, j, numshades;

    data = grp_data("palette.dat", &size);
    if (!data || size < 768)
    {
        fail("PALETTE.DAT missing or truncated");
    }
    for (i = 0; i < 768; i++)
    {
        palette24[i] = (uint8_t)(data[i] * 4 <= 255 ? data[i] * 4 : 255);
    }
    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < 256; j++)
        {
            shade_tab[i][j] = (uint8_t)j;
        }
    }
    if (size >= 770)
    {
        numshades = (int)rd16(data + 768);
        if (numshades > 32)
        {
            numshades = 32;
        }
        for (i = 0; i < numshades && 770 + i * 256 + 256 <= size; i++)
        {
            memcpy(shade_tab[i], data + 770 + i * 256, 256);
        }
    }
}

/* LOOKUP.DAT: count byte, then {pal byte, 256 remap bytes} records. The
   engine reads exactly this layout, so the file is also copied verbatim. */

static void load_lookup(void)
{
    const uint8_t *data;
    int size = 0, i, j;

    for (i = 0; i < 256; i++)
    {
        for (j = 0; j < 256; j++)
        {
            remap_tab[i][j] = (uint8_t)j;
        }
    }
    data = grp_data("lookup.dat", &size);
    if (!data || size <= 0)
    {
        return;
    }
    for (i = 0; i < data[0]; i++)
    {
        int base = 1 + i * 257;
        if (base + 257 > size)
        {
            break;
        }
        memcpy(remap_tab[data[base]], data + base + 1, 256);
    }
}

/* ---------------------------------------------------------------------------
   Output writers
   --------------------------------------------------------------------------- */

/* dnrender.cpp TileImage reads pixel bytes from offset 100 and only checks
   the file is at least 100 + width*height bytes. Mip levels are never used
   for these textures, so none are stored. */

static void write_wal(const char *path, int tile, int w, int h,
                     const uint8_t *pixels)
{
    uint8_t *out = (uint8_t *)malloc((size_t)(100 + w * h));
    char name[9];

    if (!out)
    {
        fail("out of memory");
    }
    memset(out, 0, 100);
    snprintf(name, sizeof(name), "tile%04d", tile);
    memcpy(out, name, strlen(name));
    put32(out + 32, (unsigned long)w);
    put32(out + 36, (unsigned long)h);
    memcpy(out + 100, pixels, (size_t)(w * h));
    write_file(path, out, (size_t)(100 + w * h));
    free(out);
}

/* dnpcx.cpp DN_DecodePCX expects: manufacturer 10, version 5, RLE, 8 bits,
   one plane, a stride no run may cross, and the palette marker 0x0C exactly
   769 bytes before the end of the file. */

static void write_pcx(const char *path, int w, int h, const uint8_t *pixels)
{
    uint8_t *out = (uint8_t *)malloc((size_t)w * h * 2 + 128 + 769);
    size_t len = 0;
    int x, y;

    if (!out)
    {
        fail("out of memory");
    }
    memset(out, 0, 128);
    out[0] = 10;
    out[1] = 5;
    out[2] = 1;
    out[3] = 8;
    put16(out + 4, 0);
    put16(out + 6, 0);
    put16(out + 8, (unsigned)w - 1);
    put16(out + 10, (unsigned)h - 1);
    out[65] = 1;
    put16(out + 66, (unsigned)w);
    len = 128;
    for (y = 0; y < h; y++)
    {
        const uint8_t *row = pixels + (size_t)y * w;
        x = 0;
        while (x < w)
        {
            int value = row[x], run = 1;
            while (run < 63 && x + run < w && row[x + run] == value)
            {
                run++;
            }
            if (run > 1 || value >= 192)
            {
                out[len++] = (uint8_t)(192 | run);
            }
            out[len++] = (uint8_t)value;
            x += run;
        }
    }
    out[len++] = 12;
    memcpy(out + len, palette24, 768);
    len += 768;
    write_file(path, out, len);
    free(out);
}

static void write_wav(const char *path, const uint8_t *samples, int count,
                      int rate)
{
    uint8_t header[44];
    unsigned long data_size = (unsigned long)count;
    unsigned long rate_l = (unsigned long)rate;
    FILE *f;

    memcpy(header, "RIFF", 4);
    put32(header + 4, 4 + 8 + 16 + 8 + data_size);
    memcpy(header + 8, "WAVE", 4);
    memcpy(header + 12, "fmt ", 4);
    put32(header + 16, 16);
    put16(header + 20, 1);           /* PCM */
    put16(header + 22, 1);           /* mono */
    put32(header + 24, rate_l);
    put32(header + 28, rate_l);      /* one 8-bit sample per byte */
    put16(header + 32, 1);
    put16(header + 34, 8);
    memcpy(header + 36, "data", 4);
    put32(header + 40, data_size);
    f = fopen(path, "wb");
    if (!f || fwrite(header, 1, 44, f) != 44 ||
        (count && fwrite(samples, 1, (size_t)count, f) != (size_t)count))
    {
        fail("cannot write output file");
    }
    fclose(f);
}

/* VOC blocks: type 1 and 8 carry a rate byte plus 8-bit PCM, type 9 a
   32-bit rate plus PCM. Silence and loop marker blocks carry no samples. */

static int voc_to_wav(const char *name, uint8_t **samples_out,
                      int *rate_out, int *count_out)
{
    int size = 0;
    const uint8_t *data = grp_data(name, &size);
    const uint8_t *p, *end;
    int rate = 0, count = 0;
    uint8_t *samples;

    if (!data || size < 26 || memcmp(data, "Creative Voice File", 19) != 0)
    {
        return 0;
    }
    samples = (uint8_t *)malloc((size_t)size);
    if (!samples)
    {
        fail("out of memory");
    }
    p = data + rd16(data + 20);
    end = data + size;
    while (p + 4 <= end && p[0] != 0)
    {
        int block_size = p[1] | (p[2] << 8) | (p[3] << 16);
        const uint8_t *body = p + 4;
        const uint8_t *body_end = body + block_size;

        if (body_end > end)
        {
            body_end = end;
        }
        if ((p[0] == 1 || p[0] == 8) && body + 2 <= body_end)
        {
            if (!rate && 256 - body[0] > 0)
            {
                rate = 1000000 / (256 - body[0]);
            }
            memcpy(samples + count, body + 2, (size_t)(body_end - body - 2));
            count += (int)(body_end - body - 2);
        }
        else if (p[0] == 9 && body + 4 <= body_end)
        {
            if (!rate)
            {
                rate = (int)rd32(body);
            }
            memcpy(samples + count, body + 4, (size_t)(body_end - body - 4));
            count += (int)(body_end - body - 4);
        }
        p = body_end;
    }
    if (!count || !rate)
    {
        free(samples);
        return 0;
    }
    *samples_out = samples;
    *rate_out = rate;
    *count_out = count;
    return 1;
}

/* Indexed as r | g<<5 | b<<11 over 5/6/5-bit channels, matching
   GL_BuildPalettedTexture in gl_image.cpp. Index 255 is transparent in the
   engine, so it is never picked as the nearest color. */

static void write_16to8(const char *path)
{
    uint8_t *table = (uint8_t *)malloc(65536);
    int r, g, b, i;

    if (!table)
    {
        fail("out of memory");
    }
    for (b = 0; b < 32; b++)
    {
        int b8 = (b << 3) | (b >> 2);
        for (g = 0; g < 64; g++)
        {
            int g8 = (g << 2) | (g >> 4);
            for (r = 0; r < 32; r++)
            {
                int r8 = (r << 3) | (r >> 2);
                int best = 0, best_dist = 1 << 30;
                for (i = 0; i < 255; i++)
                {
                    int dr = r8 - palette24[i * 3];
                    int dg = g8 - palette24[i * 3 + 1];
                    int db = b8 - palette24[i * 3 + 2];
                    int dist = dr * dr + dg * dg + db * db;
                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best = i;
                    }
                }
                table[r | (g << 5) | (b << 11)] = (uint8_t)best;
            }
        }
    }
    write_file(path, table, 65536);
    free(table);
}

/* LoadTGA in gl_image.cpp only accepts uncompressed type-2 24 or 32-bit
   images without colormaps and reads rows bottom-up, so pixels are written
   in BGRA order from the last row to the first. */

static void write_tga32(const char *path, int w, int h, const uint32_t *pixels)
{
    uint8_t header[18];
    uint8_t *out = (uint8_t *)malloc((size_t)w * h * 4 + 18);
    size_t len = 0;
    int x, y;

    if (!out)
    {
        fail("out of memory");
    }
    memset(header, 0, sizeof(header));
    header[2] = 2;
    put16(header + 12, (unsigned)w);
    put16(header + 14, (unsigned)h);
    header[16] = 32;
    header[17] = 8; /* 8 attribute (alpha) bits, bottom-up origin */
    memcpy(out, header, 18);
    len = 18;
    for (y = h - 1; y >= 0; y--)
    {
        const uint32_t *row = pixels + (size_t)y * w;
        for (x = 0; x < w; x++)
        {
            uint32_t p = row[x];
            out[len++] = (uint8_t)((p >> 16) & 0xff); /* blue */
            out[len++] = (uint8_t)((p >> 8) & 0xff);  /* green */
            out[len++] = (uint8_t)(p & 0xff);         /* red */
            out[len++] = (uint8_t)((p >> 24) & 0xff); /* alpha */
        }
    }
    write_file(path, out, len);
    free(out);
}

/* ---------------------------------------------------------------------------
   Tile RGBA helpers for the intro images
   --------------------------------------------------------------------------- */

static uint32_t pack_rgb(int r, int g, int b, int a)
{
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) |
           (uint32_t)r;
}

static uint32_t *tile_rgba(int tile, int pal, int shade)
{
    uint8_t *pixels = tile_pix[tile];
    uint32_t *out;
    int w, h, i;

    if (!pixels || !tile_w[tile] || !tile_h[tile])
    {
        return NULL;
    }
    w = tile_w[tile];
    h = tile_h[tile];
    out = (uint32_t *)malloc((size_t)w * h * 4);
    if (!out)
    {
        fail("out of memory");
    }
    for (i = 0; i < w * h; i++)
    {
        int c = pixels[i];
        if (c == 255)
        {
            out[i] = 0; /* fully transparent */
        }
        else
        {
            int index = shade_tab[shade][remap_tab[pal][c]];
            out[i] = pack_rgb(palette24[index * 3], palette24[index * 3 + 1],
                              palette24[index * 3 + 2], 255);
        }
    }
    return out;
}

static void blit(uint32_t *dst, const uint32_t *src, int w, int h, int x, int y)
{
    int sx, sy;

    for (sy = 0; sy < h; sy++)
    {
        int dy = y + sy;
        if (dy < 0 || dy >= SCREEN_H)
        {
            continue;
        }
        for (sx = 0; sx < w; sx++)
        {
            int dx = x + sx;
            uint32_t p;
            int a, r, g, b;

            if (dx < 0 || dx >= SCREEN_W)
            {
                continue;
            }
            p = src[sy * w + sx];
            a = (int)((p >> 24) & 0xff);
            if (!a)
            {
                continue;
            }
            if (a == 255)
            {
                dst[dy * SCREEN_W + dx] = p;
            }
            else
            {
                uint32_t d = dst[dy * SCREEN_W + dx];
                int da = (int)((d >> 24) & 0xff);
                r = (int)((p & 0xff) * a + (d & 0xff) * (255 - a)) / 255;
                g = (int)(((p >> 8) & 0xff) * a + ((d >> 8) & 0xff) * (255 - a)) / 255;
                b = (int)(((p >> 16) & 0xff) * a + ((d >> 16) & 0xff) * (255 - a)) / 255;
                dst[dy * SCREEN_W + dx] = pack_rgb(r, g, b, da > a ? da : a);
            }
        }
    }
}

static int menu_glyph(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return 2930 + ch - '0';
    }
    if (ch >= 'A' && ch <= 'Z')
    {
        return 2940 + ch - 'A';
    }
    return -1;
}

/* ---------------------------------------------------------------------------
   Intro sequence (client/dnintro.cpp)
   --------------------------------------------------------------------------- */

/* dnintro.cpp draws logo%03d.tga for 75 ms per frame, flyby.wav at frame 1
   and bombexpl.wav at frame 19. The original animation cannot be rebuilt
   from the group file, so the frames show the title composition fading in
   over LOGO_FADE_FRAMES frames. Title elements sit where TitleTile() draws
   them on the real title screen. */

static const int title_letters[3] = {2497, 2498, 2502};
static const int title_rows[3] = {104, 129, 151};

static void write_intro(const char *dir)
{
    static const char realms_text[] = "3D REALMS";
    uint32_t *screen = (uint32_t *)malloc((size_t)SCREEN_W * SCREEN_H * 4);
    uint32_t *letters[3] = {NULL, NULL, NULL};
    char path[PATH_LEN];
    int i, frame;

    if (!screen)
    {
        fail("out of memory");
    }
    for (i = 0; i < SCREEN_W * SCREEN_H; i++)
    {
        screen[i] = pack_rgb(0, 0, 0, 255);
    }

    /* Title background: the same tile the menu uses. */
    {
        uint32_t *background = tile_rgba(2456, 0, 0);
        if (background)
        {
            blit(screen, background, tile_w[2456], tile_h[2456], 0, 0);
            free(background);
        }
    }
    for (i = 0; i < 3; i++)
    {
        int tile = title_letters[i];
        letters[i] = tile_rgba(tile, 0, 0);
        if (letters[i])
        {
            blit(screen, letters[i], tile_w[tile], tile_h[tile],
                 (SCREEN_W - tile_w[tile]) / 2, title_rows[i] - tile_h[tile] / 2);
        }
    }

    for (frame = 1; frame <= LOGO_FRAMES; frame++)
    {
        int alpha = frame <= LOGO_FADE_FRAMES ? frame * 255 / LOGO_FADE_FRAMES : 255;
        uint32_t *pixels = (uint32_t *)malloc((size_t)SCREEN_W * SCREEN_H * 4);

        if (!pixels)
        {
            fail("out of memory");
        }
        for (i = 0; i < SCREEN_W * SCREEN_H; i++)
        {
            uint32_t p = screen[i];
            pixels[i] = pack_rgb((int)((p & 0xff) * alpha / 255),
                                 (int)(((p >> 8) & 0xff) * alpha / 255),
                                 (int)(((p >> 16) & 0xff) * alpha / 255), 255);
        }
        snprintf(path, sizeof(path), "%s/logo%03d.tga", dir, frame);
        write_tga32(path, SCREEN_W, SCREEN_H, pixels);
        free(pixels);
    }

    /* realms.tga: the studio name in the big menu font on a black screen. */
    {
        int total = 0, x, y = 92;
        const char *ch;
        uint32_t *realms = (uint32_t *)malloc((size_t)SCREEN_W * SCREEN_H * 4);

        if (!realms)
        {
            fail("out of memory");
        }
        for (i = 0; i < SCREEN_W * SCREEN_H; i++)
        {
            realms[i] = pack_rgb(0, 0, 0, 255);
        }
        for (ch = realms_text; *ch; ch++)
        {
            int tile = menu_glyph(*ch);
            total += tile >= 0 && tile_w[tile] ? tile_w[tile] - 1 : 6;
        }
        x = (SCREEN_W - total) / 2;
        for (ch = realms_text; *ch; ch++)
        {
            int tile = menu_glyph(*ch);
            uint32_t *glyph = tile >= 0 ? tile_rgba(tile, 0, 0) : NULL;
            if (glyph)
            {
                blit(realms, glyph, tile_w[tile], tile_h[tile], x, y);
                free(glyph);
                x += tile_w[tile] - 1;
            }
            else
            {
                x += 6;
            }
        }
        snprintf(path, sizeof(path), "%s/realms.tga", dir);
        write_tga32(path, SCREEN_W, SCREEN_H, realms);
        free(realms);
    }

    snprintf(path, sizeof(path), "%s/title.tga", dir);
    write_tga32(path, SCREEN_W, SCREEN_H, screen);

    for (i = 0; i < 3; i++)
    {
        if (letters[i])
        {
            snprintf(path, sizeof(path), "%s/tile%d.tga", dir, title_letters[i]);
            write_tga32(path, tile_w[title_letters[i]], tile_h[title_letters[i]],
                        letters[i]);
            free(letters[i]);
        }
    }

    /* fade00 is fully transparent, fade15 fully black. */
    for (i = 0; i < 16; i++)
    {
        uint32_t *pixels = (uint32_t *)malloc((size_t)SCREEN_W * SCREEN_H * 4);
        int p;

        if (!pixels)
        {
            fail("out of memory");
        }
        for (p = 0; p < SCREEN_W * SCREEN_H; p++)
        {
            pixels[p] = pack_rgb(0, 0, 0, i * 17);
        }
        snprintf(path, sizeof(path), "%s/fade%02d.tga", dir, i);
        write_tga32(path, SCREEN_W, SCREEN_H, pixels);
        free(pixels);
    }

    snprintf(path, sizeof(path), "%s/frames.txt", dir);
    write_file(path, "26\n", 3);
    free(screen);
}

/* ---------------------------------------------------------------------------
   UI tile baking
   --------------------------------------------------------------------------- */

/* The client draws dn/tile%04d_p%d_s%d as plain images, so pal and shade
   must be baked into the pixels with the same lookup order dnrender.cpp
   TileImage uses: shade[remap[pal][c]], index 255 kept transparent. */

static int write_ui_tile(const char *dir, int tile, int pal, int shade)
{
    uint8_t *pixels = tile_pix[tile];
    uint8_t *baked;
    char path[PATH_LEN];
    int w, h, i, count;

    if (!pixels || !tile_w[tile] || !tile_h[tile])
    {
        return 0;
    }
    w = tile_w[tile];
    h = tile_h[tile];
    count = w * h;
    baked = (uint8_t *)malloc((size_t)count);
    if (!baked)
    {
        fail("out of memory");
    }
    for (i = 0; i < count; i++)
    {
        baked[i] = pixels[i] == 255
                       ? 255
                       : shade_tab[shade][remap_tab[pal][pixels[i]]];
    }
    snprintf(path, sizeof(path), "%s/tile%04d_p%d_s%d.pcx", dir, tile, pal, shade);
    write_pcx(path, w, h, baked);
    free(baked);
    return 1;
}

static int write_ui_tiles(const char *dir)
{
    int written = 0;
    int tile, pal, shade;

    /* Menu alphabet, drawn plain or unselected (dnmenu.cpp, dnsavemenu.cpp). */
    for (shade = 0; shade <= 16; shade += 16)
    {
        written += write_ui_tile(dir, 2929, 0, shade);
        for (tile = 2930; tile <= 2965; tile++)
        {
            written += write_ui_tile(dir, tile, 0, shade);
        }
        for (tile = 3002; tile <= 3007; tile++)
        {
            written += write_ui_tile(dir, tile, 0, shade);
        }
        written += write_ui_tile(dir, 3022, 0, shade);
        /* Intermission text alphabet (dnintermission.cpp). */
        for (tile = 2822; tile <= 2916; tile++)
        {
            written += write_ui_tile(dir, tile, 0, shade);
        }
    }
    /* Menu cursor frames, panel strips and save-menu header. */
    for (tile = 2813; tile <= 2819; tile++)
    {
        written += write_ui_tile(dir, tile, 0, 0);
    }
    written += write_ui_tile(dir, 2456, 0, 16);
    written += write_ui_tile(dir, 2457, 0, 16);
    /* LOADSCREEN, the backdrop dofrontscreens() draws on level load. */
    written += write_ui_tile(dir, 3281, 0, 0);
    for (tile = 3240; tile <= 3242; tile++)
    {
        written += write_ui_tile(dir, tile, 0, 0);
    }
    for (tile = 3245; tile <= 3247; tile++)
    {
        written += write_ui_tile(dir, tile, 0, 0);
    }
    /* Mini font: the save menu uses pal 0, the frag bar passes the server
       player palette through, so cover every remap palette (0..23). */
    for (pal = 0; pal < 24; pal++)
    {
        for (tile = 3072; tile <= 3166; tile++)
        {
            written += write_ui_tile(dir, tile, pal, 0);
        }
    }
    /* Status bar (dnhud.cpp). */
    for (tile = 2472; tile <= 2481; tile++)
    {
        written += write_ui_tile(dir, tile, 0, 0);
    }
    written += write_ui_tile(dir, 2465, 0, 0);
    written += write_ui_tile(dir, 2465, 0, 4);
    written += write_ui_tile(dir, 2462, 0, 4);
    for (pal = 0; pal <= 23; pal += pal == 0 ? 21 : 2)
    {
        written += write_ui_tile(dir, 2471, pal, 0);
    }
    written += write_ui_tile(dir, 30, 21, 0);
    written += write_ui_tile(dir, 31, 21, 0);
    written += write_ui_tile(dir, 33, 21, 0);
    {
        static const int icons[7] = {2460, 2461, 2463, 2467, 2468, 2469, 2470};
        for (tile = 0; tile < 7; tile++)
        {
            written += write_ui_tile(dir, icons[tile], 0, 0);
        }
    }
    /* Ammo counters: shades 0/12/21 plus the pal-7 labels with 0/2/11. */
    {
        static const int row_shades[3] = {0, 12, 21};
        static const int label_shades[3] = {0, 2, 11};
        for (shade = 0; shade < 3; shade++)
        {
            for (tile = 3010; tile <= 3019; tile++)
            {
                written += write_ui_tile(dir, tile, 0, row_shades[shade]);
                written += write_ui_tile(dir, tile, 7, label_shades[shade]);
            }
            written += write_ui_tile(dir, 3020, 0, row_shades[shade]);
            written += write_ui_tile(dir, 3021, 0, row_shades[shade]);
        }
    }
    return written;
}

/* ---------------------------------------------------------------------------
   Main conversion
   --------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *root = ".";
    char path[PATH_LEN];
    char skipped[64][13];
    int skipped_count = 0;
    int i, counts_maps = 0, counts_scripts = 0, counts_wal = 0;
    int counts_ui = 0, counts_sounds = 0;
    FILE *f;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--root") && i + 1 < argc)
        {
            root = argv[++i];
        }
    }
    snprintf(path, sizeof(path), "%s/3drealms/duke3d.grp", root);
    load_grp(path);
    load_palette();
    load_lookup();
    load_art();

    /* Raw maps and CON scripts: the engine loads them from maps/ and
       scripts/ in lowercase, unchanged. */
    snprintf(path, sizeof(path), "%s/basedn/maps", root);
    make_dirs(path);
    snprintf(path, sizeof(path), "%s/basedn/scripts", root);
    make_dirs(path);
    for (i = 0; i < file_count; i++)
    {
        const char *name = file_name[i];
        if (ends_with(name, ".map"))
        {
            snprintf(path, sizeof(path), "%s/basedn/maps/%s", root, name);
            write_file(path, grp + file_offset[i], (size_t)file_size[i]);
            counts_maps++;
        }
        else if (ends_with(name, ".con"))
        {
            snprintf(path, sizeof(path), "%s/basedn/scripts/%s", root, name);
            write_file(path, grp + file_offset[i], (size_t)file_size[i]);
            counts_scripts++;
        }
    }

    /* Tile metadata: 6144 packed {int16 width, int16 height, uint32 picanm}
       entries (dnrender.cpp and dnhud.cpp read exactly this size). */
    snprintf(path, sizeof(path), "%s/basedn/sprites", root);
    make_dirs(path);
    {
        uint8_t *tiles = (uint8_t *)calloc(MAX_TILES, 8);
        if (!tiles)
        {
            fail("out of memory");
        }
        for (i = 0; i < MAX_TILES; i++)
        {
            if (tile_w[i] || tile_h[i])
            {
                put16(tiles + i * 8, tile_w[i]);
                put16(tiles + i * 8 + 2, tile_h[i]);
                put32(tiles + i * 8 + 4, tile_anim[i]);
            }
        }
        snprintf(path, sizeof(path), "%s/basedn/sprites/tiles.dat", root);
        write_file(path, tiles, MAX_TILES * 8);
        free(tiles);
    }

    /* TABLES.DAT verbatim: dnmath.cpp reads the sine and radar arrays
       straight from it. */
    {
        const uint8_t *tables = grp_data("tables.dat", &i);
        if (!tables || i < 4096 + 1280)
        {
            fail("TABLES.DAT missing or shorter than the sine/radar arrays");
        }
        snprintf(path, sizeof(path), "%s/basedn/sprites/tables.dat", root);
        write_file(path, tables, (size_t)i);
    }

    /* pics/: shade tables, palette remaps, the global palette and the 16-bit
       fallback table used by GL_BuildPalettedTexture. */
    {
        uint8_t *palookup = (uint8_t *)malloc(2 + 32 * 256);
        const uint8_t *lookup = grp_data("lookup.dat", &i);
        if (!palookup)
        {
            fail("out of memory");
        }
        put16(palookup, 32);
        memcpy(palookup + 2, shade_tab, 32 * 256);
        snprintf(path, sizeof(path), "%s/basedn/pics", root);
        make_dirs(path);
        snprintf(path, sizeof(path), "%s/basedn/pics/palookup.dat", root);
        write_file(path, palookup, 2 + 32 * 256);
        free(palookup);
        if (lookup)
        {
            snprintf(path, sizeof(path), "%s/basedn/pics/lookup.dat", root);
            write_file(path, lookup, (size_t)i);
        }
        snprintf(path, sizeof(path), "%s/basedn/pics/colormap.pcx", root);
        {
            uint8_t identity[256];
            for (i = 0; i < 256; i++)
            {
                identity[i] = (uint8_t)i;
            }
            write_pcx(path, 256, 1, identity);
        }
        snprintf(path, sizeof(path), "%s/basedn/pics/16to8.dat", root);
        write_16to8(path);
    }

    /* One raw palette-index WAL per tile: dnrender.cpp bakes pal/shade from
       these at load time. */
    snprintf(path, sizeof(path), "%s/basedn/textures/duke", root);
    make_dirs(path);
    clear_generated(path);
    for (i = 0; i < MAX_TILES; i++)
    {
        if (tile_pix[i] && tile_w[i] && tile_h[i])
        {
            char wal[PATH_LEN];
            snprintf(wal, sizeof(wal), "%s/basedn/textures/duke/tile%04d.wal", root, i);
            write_wal(wal, i, tile_w[i], tile_h[i], tile_pix[i]);
            counts_wal++;
        }
    }

    snprintf(path, sizeof(path), "%s/basedn/pics/dn", root);
    make_dirs(path);
    clear_generated(path);
    counts_ui = write_ui_tiles(path);

    /* Every VOC becomes an 8-bit PCM WAV with its original rate. */
    snprintf(path, sizeof(path), "%s/basedn/sound", root);
    make_dirs(path);
    for (i = 0; i < file_count; i++)
    {
        uint8_t *samples;
        char stem[13];
        int rate, count;

        if (!ends_with(file_name[i], ".voc"))
        {
            continue;
        }
        if (!voc_to_wav(file_name[i], &samples, &rate, &count))
        {
            if (skipped_count < 64)
            {
                strcpy(skipped[skipped_count++], file_name[i]);
            }
            continue;
        }
        if (rate <= 192000)
        {
            strcpy(stem, file_name[i]);
            stem[strlen(stem) - 4] = '\0'; /* drop ".voc" */
            snprintf(path, sizeof(path), "%s/basedn/sound/%s.wav", root, stem);
            write_wav(path, samples, count, rate);
            counts_sounds++;
        }
        else if (skipped_count < 64)
        {
            strcpy(skipped[skipped_count++], file_name[i]);
        }
        free(samples);
    }

    snprintf(path, sizeof(path), "%s/basedn/anm/dnintro", root);
    make_dirs(path);
    clear_generated(path);
    write_intro(path);

    snprintf(path, sizeof(path), "%s/basedn/asset-manifest.json", root);
    f = fopen(path, "w");
    if (!f)
    {
        fail("cannot write asset-manifest.json");
    }
    fprintf(f, "{\n");
    fprintf(f, "  \"source\": \"3drealms/duke3d.grp\",\n");
    fprintf(f, "  \"generated\": {\n");
    fprintf(f, "    \"maps\": %d,\n", counts_maps);
    fprintf(f, "    \"scripts\": %d,\n", counts_scripts);
    fprintf(f, "    \"wal_textures\": %d,\n", counts_wal);
    fprintf(f, "    \"ui_tiles\": %d,\n", counts_ui);
    fprintf(f, "    \"sounds\": %d\n", counts_sounds);
    fprintf(f, "  },\n");
    fprintf(f, "  \"notes\": [\n");
    fprintf(f, "    \"Music is not converted: the engine only plays music/<name>.mp3 and the group file holds MIDI tracks.\",\n");
    fprintf(f, "    \"DUKE.RTS taunt sounds are not read by the engine and are skipped.\",\n");
    fprintf(f, "    \"Deathmatch frag-bar names cover palettes 0..23; higher values draw no name.\"\n");
    fprintf(f, "  ],\n");
    fprintf(f, "  \"skipped_voc\": [");
    for (i = 0; i < skipped_count; i++)
    {
        fprintf(f, "%s\n    \"%s\"", i ? "," : "", skipped[i]);
    }
    fprintf(f, "%s]\n", skipped_count ? "\n  " : "");
    fprintf(f, "}\n");
    fclose(f);

    printf("convertgrp: %d maps, %d wal, %d ui tiles, %d sounds\n", counts_maps,
           counts_wal, counts_ui, counts_sounds);
    return 0;
}
