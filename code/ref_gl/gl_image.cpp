/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "gl_local.h"
#include "../qcommon/dnpcx.h"

image_t gltextures[MAX_GLTEXTURES];
int numgltextures;

static byte intensitytable[256];
static unsigned char gammatable[256];

cvar_t *intensity;

unsigned d_8to24table[256];

qboolean GL_Upload8(byte *data, int width, int height, qboolean mipmap);
qboolean GL_Upload32(unsigned *data, int width, int height, qboolean mipmap);

int gl_filter_min = GL_NEAREST;
int gl_filter_max = GL_NEAREST;

/*
====================================================================

TEXTURE HANDLES

The engine addresses textures by slot number (TEXNUM_*). The core
profile requires real GL names, so a fixed pool of them is generated
once and the slot numbers index into it.

====================================================================
*/

static GLuint r_texture_handles[TEXNUM_IMAGES + MAX_GLTEXTURES];

void GL_GenTextureHandles(void)
{
    glGenTextures(sizeof(r_texture_handles) / sizeof(r_texture_handles[0]), r_texture_handles);
}

GLuint R_TextureHandle(int texnum)
{
    if (texnum < 0 || texnum >= (int)(sizeof(r_texture_handles) / sizeof(r_texture_handles[0])))
    {
        ri.Sys_Error(ERR_DROP, "R_TextureHandle: bad texture slot %d", texnum);
    }
    return r_texture_handles[texnum];
}

/*
====================================================================

BINDING

====================================================================
*/

void GL_SelectTexture(GLenum unit)
{
    int tmu = (unit == GL_TEXTURE1) ? 1 : 0;

    if (tmu == gl_state.currenttmu)
    {
        return;
    }

    gl_state.currenttmu = tmu;
    glActiveTexture(GL_TEXTURE0 + tmu);
}

void GL_EnableMultitexture(qboolean enable)
{
    if (!enable)
    {
        // The lightmap sampler always runs, so park unit 1 on the white
        // texture when no lightmap is bound.
        GL_SelectTexture(GL_TEXTURE1);
        GL_Bind(TEXNUM_WHITE);
    }
    GL_SelectTexture(GL_TEXTURE0);
}

void GL_Bind(int texnum)
{
    extern image_t *draw_chars;

    if (gl_nobind->value && draw_chars) // performance evaluation option
    {
        texnum = draw_chars->texnum;
    }
    if (gl_state.currenttextures[gl_state.currenttmu] == texnum)
    {
        return;
    }
    gl_state.currenttextures[gl_state.currenttmu] = texnum;
    glBindTexture(GL_TEXTURE_2D, R_TextureHandle(texnum));
}

void GL_MBind(GLenum target, int texnum)
{
    GL_SelectTexture(target);
    if (gl_state.currenttextures[gl_state.currenttmu] == texnum)
    {
        return;
    }
    GL_Bind(texnum);
}

typedef struct
{
    char *name;
    int minimize, maximize;
} glmode_t;

glmode_t modes[] = {{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
                    {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
                    {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
                    {"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
                    {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
                    {"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}};

#define NUM_GL_MODES (sizeof(modes) / sizeof(glmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode(char *string)
{
    int i;
    image_t *glt;

    for (i = 0; i < NUM_GL_MODES; i++)
    {
        if (!Q_stricmp(modes[i].name, string))
        {
            break;
        }
    }

    if (i == NUM_GL_MODES)
    {
        ri.Con_Printf(PRINT_ALL, "bad filter name\n");
        return;
    }

    gl_filter_min = modes[i].minimize;
    gl_filter_max = modes[i].maximize;

    // change all the existing mipmap texture objects
    for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
    {
        if (glt->texnum)
        {
            GL_Bind(glt->texnum);
            int console = !strcmp(glt->name, "pics/conchars.pcx");
            int ui = glt->type == it_pic || glt->type == it_sky || glt->type == it_duke;
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                             console ? GL_LINEAR
                             : ui    ? GL_NEAREST
                                     : gl_filter_min);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                             console ? GL_LINEAR
                             : ui    ? GL_NEAREST
                                     : gl_filter_max);
        }
    }
}

/*
===============
GL_ImageList_f
===============
*/
void GL_ImageList_f(void)
{
    int i;
    image_t *image;
    int texels;

    ri.Con_Printf(PRINT_ALL, "------------------\n");
    texels = 0;

    for (i = 0, image = gltextures; i < numgltextures; i++, image++)
    {
        if (image->texnum <= 0)
        {
            continue;
        }
        texels += image->upload_width * image->upload_height;
        switch (image->type)
        {
        case it_skin:
            ri.Con_Printf(PRINT_ALL, "M");
            break;
        case it_sprite:
            ri.Con_Printf(PRINT_ALL, "S");
            break;
        case it_wall:
            ri.Con_Printf(PRINT_ALL, "W");
            break;
        case it_pic:
            ri.Con_Printf(PRINT_ALL, "P");
            break;
        default:
            ri.Con_Printf(PRINT_ALL, " ");
            break;
        }

        ri.Con_Printf(PRINT_ALL, " %3i %3i: %s\n", image->upload_width, image->upload_height,
                      image->name);
    }
    ri.Con_Printf(PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define MAX_SCRAPS 1
#define BLOCK_WIDTH 256
#define BLOCK_HEIGHT 256

int scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT];
qboolean scrap_dirty;

// returns a texture number and the position inside it
int Scrap_AllocBlock(int w, int h, int *x, int *y)
{
    int i, j;
    int best, best2;
    int texnum;

    for (texnum = 0; texnum < MAX_SCRAPS; texnum++)
    {
        best = BLOCK_HEIGHT;

        for (i = 0; i < BLOCK_WIDTH - w; i++)
        {
            best2 = 0;

            for (j = 0; j < w; j++)
            {
                if (scrap_allocated[texnum][i + j] >= best)
                {
                    break;
                }
                if (scrap_allocated[texnum][i + j] > best2)
                {
                    best2 = scrap_allocated[texnum][i + j];
                }
            }
            if (j == w)
            { // this is a valid spot
                *x = i;
                *y = best = best2;
            }
        }

        if (best + h > BLOCK_HEIGHT)
        {
            continue;
        }

        for (i = 0; i < w; i++)
        {
            scrap_allocated[texnum][*x + i] = best + h;
        }

        return texnum;
    }

    return -1;
    //	Sys_Error ("Scrap_AllocBlock: full");
}

int scrap_uploads;

void Scrap_Upload(void)
{
    scrap_uploads++;
    GL_Bind(TEXNUM_SCRAPS);
    GL_Upload8(scrap_texels[0], BLOCK_WIDTH, BLOCK_HEIGHT, false);
    scrap_dirty = false;
}

/*
=================================================================

PCX LOADING

=================================================================
*/

/*
==============
LoadPCX
==============
*/
void LoadPCX(char *filename, byte **pic, byte **palette, int *width, int *height)
{
    byte *data, colors[768];
    int w, h;
    *pic = NULL;
    if (palette)
    {
        *palette = NULL;
    }
    int length = ri.FS_LoadFile(filename, (void **)&data);
    if (!data)
    {
        return;
    }
    int valid = DN_DecodePCX(data, length > 0 ? (size_t)length : 0, pic, &w, &h, colors);
    ri.FS_FreeFile(data);
    if (!valid)
    {
        ri.Con_Printf(PRINT_ALL, "Malformed PCX: %s\n", filename);
        return;
    }
    if (palette)
    {
        *palette = (byte *)(malloc(sizeof(colors)));
        if (!*palette)
        {
            free(*pic);
            *pic = NULL;
            ri.Sys_Error(ERR_DROP, "Cannot allocate PCX palette");
            return;
        }
        memcpy(*palette, colors, sizeof(colors));
    }
    if (width)
    {
        *width = w;
    }
    if (height)
    {
        *height = h;
    }
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader
{
    unsigned char id_length, colormap_type, image_type;
    unsigned short colormap_index, colormap_length;
    unsigned char colormap_size;
    unsigned short x_origin, y_origin, width, height;
    unsigned char pixel_size, attributes;
} TargaHeader;

/*
=============
LoadTGA
=============
*/
void LoadTGA(char *name, byte **pic, int *width, int *height)
{
    int columns, rows, numPixels;
    byte *pixbuf;
    int row, column;
    byte *buf_p;
    byte *buffer;
    int length;
    TargaHeader targa_header;
    byte *targa_rgba;
    byte tmp[2];

    *pic = NULL;

    //
    // load the file
    //
    length = ri.FS_LoadFile(name, (void **)&buffer);
    if (!buffer)
    {
        ri.Con_Printf(PRINT_DEVELOPER, "Bad tga file %s\n", name);
        return;
    }

    buf_p = buffer;

    targa_header.id_length = *buf_p++;
    targa_header.colormap_type = *buf_p++;
    targa_header.image_type = *buf_p++;

    tmp[0] = buf_p[0];
    tmp[1] = buf_p[1];
    targa_header.colormap_index = LittleShort(*((short *)tmp));
    buf_p += 2;
    tmp[0] = buf_p[0];
    tmp[1] = buf_p[1];
    targa_header.colormap_length = LittleShort(*((short *)tmp));
    buf_p += 2;
    targa_header.colormap_size = *buf_p++;
    targa_header.x_origin = LittleShort(*((short *)buf_p));
    buf_p += 2;
    targa_header.y_origin = LittleShort(*((short *)buf_p));
    buf_p += 2;
    targa_header.width = LittleShort(*((short *)buf_p));
    buf_p += 2;
    targa_header.height = LittleShort(*((short *)buf_p));
    buf_p += 2;
    targa_header.pixel_size = *buf_p++;
    targa_header.attributes = *buf_p++;

    if (targa_header.image_type != 2 && targa_header.image_type != 10)
    {
        ri.Sys_Error(ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");
    }

    if (targa_header.colormap_type != 0 ||
        (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
    {
        ri.Sys_Error(ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
    }

    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows;

    if (width)
    {
        *width = columns;
    }
    if (height)
    {
        *height = rows;
    }

    targa_rgba = (byte *)(malloc(numPixels * 4));
    *pic = targa_rgba;

    if (targa_header.id_length != 0)
    {
        buf_p += targa_header.id_length; // skip TARGA image comment
    }

    if (targa_header.image_type == 2)
    { // Uncompressed, RGB images
        for (row = rows - 1; row >= 0; row--)
        {
            pixbuf = targa_rgba + row * columns * 4;
            for (column = 0; column < columns; column++)
            {
                unsigned char red, green, blue, alphabyte;
                switch (targa_header.pixel_size)
                {
                case 24:

                    blue = *buf_p++;
                    green = *buf_p++;
                    red = *buf_p++;
                    *pixbuf++ = red;
                    *pixbuf++ = green;
                    *pixbuf++ = blue;
                    *pixbuf++ = 255;
                    break;
                case 32:
                    blue = *buf_p++;
                    green = *buf_p++;
                    red = *buf_p++;
                    alphabyte = *buf_p++;
                    *pixbuf++ = red;
                    *pixbuf++ = green;
                    *pixbuf++ = blue;
                    *pixbuf++ = alphabyte;
                    break;
                }
            }
        }
    }
    else if (targa_header.image_type == 10)
    { // Runlength encoded RGB images
        unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;
        for (row = rows - 1; row >= 0; row--)
        {
            pixbuf = targa_rgba + row * columns * 4;
            for (column = 0; column < columns;)
            {
                packetHeader = *buf_p++;
                packetSize = 1 + (packetHeader & 0x7f);
                if (packetHeader & 0x80)
                { // run-length packet
                    switch (targa_header.pixel_size)
                    {
                    case 24:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        alphabyte = 255;
                        break;
                    case 32:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        alphabyte = *buf_p++;
                        break;
                    }

                    for (j = 0; j < packetSize; j++)
                    {
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = alphabyte;
                        column++;
                        if (column == columns)
                        { // run spans across rows
                            column = 0;
                            if (row > 0)
                            {
                                row--;
                            }
                            else
                            {
                                goto breakOut;
                            }
                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                }
                else
                { // non run-length packet
                    for (j = 0; j < packetSize; j++)
                    {
                        switch (targa_header.pixel_size)
                        {
                        case 24:
                            blue = *buf_p++;
                            green = *buf_p++;
                            red = *buf_p++;
                            *pixbuf++ = red;
                            *pixbuf++ = green;
                            *pixbuf++ = blue;
                            *pixbuf++ = 255;
                            break;
                        case 32:
                            blue = *buf_p++;
                            green = *buf_p++;
                            red = *buf_p++;
                            alphabyte = *buf_p++;
                            *pixbuf++ = red;
                            *pixbuf++ = green;
                            *pixbuf++ = blue;
                            *pixbuf++ = alphabyte;
                            break;
                        }
                        column++;
                        if (column == columns)
                        { // pixel packet run spans across rows
                            column = 0;
                            if (row > 0)
                            {
                                row--;
                            }
                            else
                            {
                                goto breakOut;
                            }
                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                }
            }
        breakOut:;
        }
    }

    ri.FS_FreeFile(buffer);
}

/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/

/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
    short x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP(off, dx, dy)                                                                \
    {                                                                                              \
        if (pos[off] == fillcolor)                                                                 \
        {                                                                                          \
            pos[off] = 255;                                                                        \
            fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy);                                      \
            inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;                                               \
        }                                                                                          \
        else if (pos[off] != 255)                                                                  \
            fdc = pos[off];                                                                        \
    }

void R_FloodFillSkin(byte *skin, int skinwidth, int skinheight)
{
    byte fillcolor = *skin; // assume this is the pixel to fill
    floodfill_t fifo[FLOODFILL_FIFO_SIZE];
    int inpt = 0, outpt = 0;
    int filledcolor = -1;
    int i;

    if (filledcolor == -1)
    {
        filledcolor = 0;
        // attempt to find opaque black
        for (i = 0; i < 256; ++i)
        {
            if (d_8to24table[i] == (255 << 0)) // alpha 1.0
            {
                filledcolor = i;
                break;
            }
        }
    }

    // can't fill to filled color or to transparent color (used as visited marker)
    if ((fillcolor == filledcolor) || (fillcolor == 255))
    {
        // printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
        return;
    }

    fifo[inpt].x = 0, fifo[inpt].y = 0;
    inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

    while (outpt != inpt)
    {
        int x = fifo[outpt].x, y = fifo[outpt].y;
        int fdc = filledcolor;
        byte *pos = &skin[x + skinwidth * y];

        outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

        if (x > 0)
        {
            FLOODFILL_STEP(-1, -1, 0);
        }
        if (x < skinwidth - 1)
        {
            FLOODFILL_STEP(1, 1, 0);
        }
        if (y > 0)
        {
            FLOODFILL_STEP(-skinwidth, 0, -1);
        }
        if (y < skinheight - 1)
        {
            FLOODFILL_STEP(skinwidth, 0, 1);
        }
        skin[x + skinwidth * y] = fdc;
    }
}

//=======================================================

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth,
                        int outheight)
{
    int i, j;
    unsigned *inrow, *inrow2;
    unsigned frac, fracstep;
    unsigned p1[1024], p2[1024];
    byte *pix1, *pix2, *pix3, *pix4;

    fracstep = inwidth * 0x10000 / outwidth;

    frac = fracstep >> 2;
    for (i = 0; i < outwidth; i++)
    {
        p1[i] = 4 * (frac >> 16);
        frac += fracstep;
    }
    frac = 3 * (fracstep >> 2);
    for (i = 0; i < outwidth; i++)
    {
        p2[i] = 4 * (frac >> 16);
        frac += fracstep;
    }

    for (i = 0; i < outheight; i++, out += outwidth)
    {
        inrow = in + inwidth * (int)((i + 0.25) * inheight / outheight);
        inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight);
        frac = fracstep >> 1;
        for (j = 0; j < outwidth; j++)
        {
            pix1 = (byte *)inrow + p1[j];
            pix2 = (byte *)inrow + p2[j];
            pix3 = (byte *)inrow2 + p1[j];
            pix4 = (byte *)inrow2 + p2[j];
            ((byte *)(out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
            ((byte *)(out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
            ((byte *)(out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
            ((byte *)(out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
        }
    }
}

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void GL_LightScaleTexture(unsigned *in, int inwidth, int inheight, qboolean only_gamma)
{
    if (only_gamma)
    {
        int i, c;
        byte *p;

        p = (byte *)in;

        c = inwidth * inheight;
        for (i = 0; i < c; i++, p += 4)
        {
            p[0] = gammatable[p[0]];
            p[1] = gammatable[p[1]];
            p[2] = gammatable[p[2]];
        }
    }
    else
    {
        int i, c;
        byte *p;

        p = (byte *)in;

        c = inwidth * inheight;
        for (i = 0; i < c; i++, p += 4)
        {
            p[0] = gammatable[intensitytable[p[0]]];
            p[1] = gammatable[intensitytable[p[1]]];
            p[2] = gammatable[intensitytable[p[2]]];
        }
    }
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap(byte *in, int width, int height)
{
    int i, j;
    byte *out;

    width <<= 2;
    height >>= 1;
    out = in;
    for (i = 0; i < height; i++, in += width)
    {
        for (j = 0; j < width; j += 8, out += 4, in += 8)
        {
            out[0] = (in[0] + in[4] + in[width + 0] + in[width + 4]) >> 2;
            out[1] = (in[1] + in[5] + in[width + 1] + in[width + 5]) >> 2;
            out[2] = (in[2] + in[6] + in[width + 2] + in[width + 6]) >> 2;
            out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
        }
    }
}

/*
===============
GL_Upload32

Returns has_alpha
===============
*/

int upload_width, upload_height;

qboolean GL_Upload32(unsigned *data, int width, int height, qboolean mipmap)
{
    unsigned scaled[256 * 256];
    int scaled_width, scaled_height;
    int i, c;
    byte *scan;
    int comp;
    qboolean alpha;

    /* OpenGL 3.3 accepts the original ART/PCX dimensions directly. The legacy
       256-pixel cap and power-of-two resampling destroy small HUD glyphs and
       stretch weapon pixels before their sampler even runs. */
    if (!mipmap && width > 0 && height > 0 && width <= 2048 && height <= 2048)
    {
        size_t count = (size_t)width * height;
        unsigned *copy = (unsigned int *)(malloc(count * sizeof(*copy)));
        qboolean has_alpha = false;
        if (!copy)
        {
            ri.Sys_Error(ERR_DROP, "Cannot allocate native texture upload");
        }
        memcpy(copy, data, count * sizeof(*copy));
        for (size_t pixel = 0; pixel < count; pixel++)
        {
            if (((byte *)&copy[pixel])[3] != 255)
            {
                has_alpha = true;
                break;
            }
        }
        GL_LightScaleTexture(copy, width, height, true);
        glTexImage2D(GL_TEXTURE_2D, 0, has_alpha ? GL_RGBA8 : GL_RGB8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, copy);
        free(copy);
        upload_width = width;
        upload_height = height;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return has_alpha;
    }

    for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
        ;
    if (gl_round_down->value && scaled_width > width && mipmap)
    {
        scaled_width >>= 1;
    }
    for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
        ;
    if (gl_round_down->value && scaled_height > height && mipmap)
    {
        scaled_height >>= 1;
    }

    // let people sample down the world textures for speed
    if (mipmap)
    {
        scaled_width >>= (int)gl_picmip->value;
        scaled_height >>= (int)gl_picmip->value;
    }

    // don't ever bother with >256 textures
    if (scaled_width > 256)
    {
        scaled_width = 256;
    }
    if (scaled_height > 256)
    {
        scaled_height = 256;
    }

    if (scaled_width < 1)
    {
        scaled_width = 1;
    }
    if (scaled_height < 1)
    {
        scaled_height = 1;
    }

    upload_width = scaled_width;
    upload_height = scaled_height;

    if (scaled_width * scaled_height > sizeof(scaled) / 4)
    {
        ri.Sys_Error(ERR_DROP, "GL_Upload32: too big");
    }

    // scan the texture for any non-255 alpha
    c = width * height;
    scan = ((byte *)data) + 3;
    alpha = false;
    for (i = 0; i < c; i++, scan += 4)
    {
        if (*scan != 255)
        {
            alpha = true;
            break;
        }
    }

    comp = alpha ? GL_RGBA8 : GL_RGB8;

    if (scaled_width == width && scaled_height == height)
    {
        if (!mipmap)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, data);
            goto done;
        }
        memcpy(scaled, data, width * height * 4);
    }
    else
    {
        GL_ResampleTexture(data, width, height, scaled, scaled_width, scaled_height);
    }

    GL_LightScaleTexture(scaled, scaled_width, scaled_height, !mipmap);

    glTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 scaled);

    if (mipmap)
    {
        int miplevel;

        miplevel = 0;
        while (scaled_width > 1 || scaled_height > 1)
        {
            GL_MipMap((byte *)scaled, scaled_width, scaled_height);
            scaled_width >>= 1;
            scaled_height >>= 1;
            if (scaled_width < 1)
            {
                scaled_width = 1;
            }
            if (scaled_height < 1)
            {
                scaled_height = 1;
            }
            miplevel++;
            glTexImage2D(GL_TEXTURE_2D, miplevel, comp, scaled_width, scaled_height, 0, GL_RGBA,
                          GL_UNSIGNED_BYTE, scaled);
        }
    }
done:

    if (mipmap)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }

    return alpha;
}

/*
===============
GL_Upload8

Returns has_alpha
===============
*/
qboolean GL_Upload8(byte *data, int width, int height, qboolean mipmap)
{
    static unsigned trans[2048 * 2048];
    int i, s;
    int p;

    s = width * height;

    if (s > sizeof(trans) / 4)
    {
        ri.Sys_Error(ERR_DROP, "GL_Upload8: too large");
    }

    for (i = 0; i < s; i++)
    {
        p = data[i];
        trans[i] = d_8to24table[p];

        if (p == 255)
        { // transparent, so scan around for another color
            // to avoid alpha fringes
            // FIXME: do a full flood fill so mips work...
            if (i > width && data[i - width] != 255)
            {
                p = data[i - width];
            }
            else if (i < s - width && data[i + width] != 255)
            {
                p = data[i + width];
            }
            else if (i > 0 && data[i - 1] != 255)
            {
                p = data[i - 1];
            }
            else if (i < s - 1 && data[i + 1] != 255)
            {
                p = data[i + 1];
            }
            else
            {
                p = 0;
            }
            // copy rgb components
            ((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
            ((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
            ((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
        }
    }

    return GL_Upload32(trans, width, height, mipmap);
}

/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic(char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
    image_t *image;
    int i;

    // find a free image_t
    for (i = 0, image = gltextures; i < numgltextures; i++, image++)
    {
        if (!image->texnum)
        {
            break;
        }
    }
    if (i == numgltextures)
    {
        if (numgltextures == MAX_GLTEXTURES)
        {
            ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
        }
        numgltextures++;
    }
    image = &gltextures[i];

    if (strlen(name) >= sizeof(image->name))
    {
        ri.Sys_Error(ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
    }
    strcpy(image->name, name);
    image->registration_sequence = registration_sequence;

    image->width = width;
    image->height = height;
    image->type = type;

    if (type == it_skin && bits == 8)
    {
        R_FloodFillSkin(pic, width, height);
    }

    // load little pics into the scrap
    if (image->type == it_pic && bits == 8 && image->width < 64 && image->height < 64)
    {
        int x, y;
        int i, j, k;
        int texnum;

        texnum = Scrap_AllocBlock(image->width, image->height, &x, &y);
        if (texnum == -1)
        {
            goto nonscrap;
        }
        scrap_dirty = true;

        // copy the texels into the scrap block
        k = 0;
        for (i = 0; i < image->height; i++)
        {
            for (j = 0; j < image->width; j++, k++)
            {
                scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = pic[k];
            }
        }
        image->texnum = TEXNUM_SCRAPS + texnum;
        image->scrap = true;
        image->has_alpha = true;
        image->sl = (x + 0.01) / (float)BLOCK_WIDTH;
        image->sh = (x + image->width - 0.01) / (float)BLOCK_WIDTH;
        image->tl = (y + 0.01) / (float)BLOCK_WIDTH;
        image->th = (y + image->height - 0.01) / (float)BLOCK_WIDTH;
    }
    else
    {
    nonscrap:
        image->scrap = false;
        image->texnum = TEXNUM_IMAGES + (image - gltextures);
        GL_Bind(image->texnum);
        if (bits == 8)
        {
            image->has_alpha = GL_Upload8(
                pic, width, height,
                (image->type != it_pic && image->type != it_sky && image->type != it_duke));
        }
        else
        {
            image->has_alpha = GL_Upload32(
                (unsigned *)pic, width, height,
                (image->type != it_pic && image->type != it_sky && image->type != it_duke));
        }
        image->upload_width = upload_width; // after power of 2 and scales
        image->upload_height = upload_height;
        image->sl = 0;
        image->sh = 1;
        image->tl = 0;
        image->th = 1;
        if (image->type == it_pic || image->type == it_sky || image->type == it_duke)
        {
            int filter = !strcmp(name, "pics/conchars.pcx") ? GL_LINEAR : GL_NEAREST;
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }
    }

    return image;
}

/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal(char *name)
{
    miptex_t *mt;
    int width, height, ofs;
    image_t *image;

    ri.FS_LoadFile(name, (void **)&mt);
    if (!mt)
    {
        ri.Con_Printf(PRINT_ALL, "GL_FindImage: can't load %s\n", name);
        return r_notexture;
    }

    width = LittleLong(mt->width);
    height = LittleLong(mt->height);
    ofs = LittleLong(mt->offsets[0]);

    image = GL_LoadPic(name, (byte *)mt + ofs, width, height, it_wall, 8);

    ri.FS_FreeFile((void *)mt);

    return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t *GL_FindImage(char *name, imagetype_t type)
{
    image_t *image;
    int i, len;
    byte *pic, *palette;
    int width, height;

    if (!name)
    {
        return NULL; //	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
    }
    len = strlen(name);
    if (len < 5)
    {
        return NULL; //	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);
    }

    // look for it
    for (i = 0, image = gltextures; i < numgltextures; i++, image++)
    {
        if (!strcmp(name, image->name))
        {
            image->registration_sequence = registration_sequence;
            return image;
        }
    }

    //
    // load the pic from disk
    //
    pic = NULL;
    palette = NULL;
    if (!strcmp(name + len - 4, ".pcx"))
    {
        LoadPCX(name, &pic, &palette, &width, &height);
        if (!pic)
        {
            return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
        }
        image = GL_LoadPic(name, pic, width, height, type, 8);
    }
    else if (!strcmp(name + len - 4, ".wal"))
    {
        image = GL_LoadWal(name);
    }
    else if (!strcmp(name + len - 4, ".tga"))
    {
        LoadTGA(name, &pic, &width, &height);
        if (!pic)
        {
            return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
        }
        image = GL_LoadPic(name, pic, width, height, type, 32);
    }
    else
    {
        return NULL; //	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name);
    }

    if (pic)
    {
        free(pic);
    }
    if (palette)
    {
        free(palette);
    }

    return image;
}

/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin(char *name)
{
    return GL_FindImage(name, it_skin);
}

/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed. The GL texture objects belong to the shared pool
and get overwritten the next time the slot is uploaded to.
================
*/
void GL_FreeUnusedImages(void)
{
    int i;
    image_t *image;

    // never free r_notexture or particle texture
    r_notexture->registration_sequence = registration_sequence;
    r_particletexture->registration_sequence = registration_sequence;

    for (i = 0, image = gltextures; i < numgltextures; i++, image++)
    {
        if (image->registration_sequence == registration_sequence)
        {
            continue; // used this sequence
        }
        if (!image->registration_sequence)
        {
            continue; // free image_t slot
        }
        if (image->type == it_pic)
        {
            continue; // don't free pics
        }
        memset(image, 0, sizeof(*image));
    }
}

/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette(void)
{
    int i;
    int r, g, b;
    unsigned v;
    byte *pic, *pal;
    int width, height;

    // get the palette

    LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);
    if (!pal)
    {
        ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");
    }

    for (i = 0; i < 256; i++)
    {
        r = pal[i * 3 + 0];
        g = pal[i * 3 + 1];
        b = pal[i * 3 + 2];

        v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
        d_8to24table[i] = LittleLong(v);
    }

    d_8to24table[255] &= LittleLong(0xffffff); // 255 is transparent

    free(pic);
    free(pal);

    return 0;
}

/*
===============
GL_InitImages
===============
*/
void GL_InitImages(void)
{
    int i, j;
    float g = vid_gamma->value;

    registration_sequence = 1;

    // init intensity conversions
    intensity = ri.Cvar_Get("intensity", "2", 0);

    if (intensity->value <= 1)
    {
        ri.Cvar_Set("intensity", "1");
    }

    gl_state.inverse_intensity = 1 / intensity->value;

    Draw_GetPalette();

    for (i = 0; i < 256; i++)
    {
        if (g == 1)
        {
            gammatable[i] = i;
        }
        else
        {
            float inf;

            inf = 255 * pow((i + 0.5) / 255.5, g) + 0.5;
            if (inf < 0)
            {
                inf = 0;
            }
            if (inf > 255)
            {
                inf = 255;
            }
            gammatable[i] = inf;
        }
    }

    for (i = 0; i < 256; i++)
    {
        j = i * intensity->value;
        if (j > 255)
        {
            j = 255;
        }
        intensitytable[i] = j;
    }
}

/*
===============
GL_ShutdownImages
===============
*/
void GL_ShutdownImages(void)
{
    numgltextures = 0;
    memset(gltextures, 0, sizeof(gltextures));
}
