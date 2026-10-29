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
// r_misc.cpp

#include "gl_local.h"

#ifdef _WIN32
#include "../win32/glw_win.h"
#endif

/*
==================
R_InitParticleTexture
==================
*/
byte dottexture[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0}, {0, 0, 1, 1, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0},
};

void R_InitParticleTexture(void)
{
    int x, y;
    byte data[8][8][4];

    //
    // particle texture
    //
    for (x = 0; x < 8; x++)
    {
        for (y = 0; y < 8; y++)
        {
            data[y][x][0] = 255;
            data[y][x][1] = 255;
            data[y][x][2] = 255;
            data[y][x][3] = dottexture[x][y] * 255;
        }
    }
    r_particletexture = GL_LoadPic("***particle***", (byte *)data, 8, 8, it_sprite, 32);

    //
    // also use this for bad textures, but without alpha
    //
    for (x = 0; x < 8; x++)
    {
        for (y = 0; y < 8; y++)
        {
            data[y][x][0] = dottexture[x & 3][y & 3] * 255;
            data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
            data[y][x][2] = 0; // dottexture[x&3][y&3]*255;
            data[y][x][3] = 255;
        }
    }
    r_notexture = GL_LoadPic("***r_notexture***", (byte *)data, 8, 8, it_wall, 32);
}

/*
==============================================================================

                        SCREEN SHOTS

==============================================================================
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
==================
GL_ScreenShot_f
==================
*/
void GL_ScreenShot_f(void)
{
    byte *buffer;
    char picname[80];
    char checkname[MAX_OSPATH];
    int i, c, temp;
    FILE *f;

    // create the scrnshots directory if it doesn't exist
    Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
    Sys_Mkdir(checkname);

    //
    // find a file name to save it to
    //

    for (i = 0; i < 10000; i++)
    {
        snprintf(picname, sizeof(picname), "dn%04d.tga", i);
        Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
        f = fopen(checkname, "rb");
        if (!f)
        {
            break; // file doesn't exist
        }
        fclose(f);
    }
    if (i == 10000)
    {
        ri.Con_Printf(PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n");
        return;
    }

    buffer = (byte *)(malloc(vid.width * vid.height * 3 + 18));
    if (!buffer)
    {
        ri.Con_Printf(PRINT_ALL, "Cannot allocate screenshot buffer\n");
        return;
    }
    memset(buffer, 0, 18);
    buffer[2] = 2; // uncompressed type
    buffer[12] = vid.width & 255;
    buffer[13] = vid.width >> 8;
    buffer[14] = vid.height & 255;
    buffer[15] = vid.height >> 8;
    buffer[16] = 24; // pixel size

    glReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);

    // swap rgb to bgr
    c = 18 + vid.width * vid.height * 3;
    for (i = 18; i < c; i += 3)
    {
        temp = buffer[i];
        buffer[i] = buffer[i + 2];
        buffer[i + 2] = temp;
    }

    f = fopen(checkname, "wb");
    if (!f)
    {
        free(buffer);
        ri.Con_Printf(PRINT_ALL, "Cannot write screenshot %s\n", checkname);
        return;
    }
    size_t written = fwrite(buffer, 1, c, f);
    int closed = fclose(f);
    if (written != (size_t)c || closed)
    {
        free(buffer);
        ri.Con_Printf(PRINT_ALL, "Screenshot write failed: %s\n", checkname);
        return;
    }

    free(buffer);
    ri.Con_Printf(PRINT_ALL, "Wrote %s\n", picname);
}

/*
** GL_Strings_f

The core profile dropped glGetString(GL_EXTENSIONS), so walk the indexed list.
*/
void GL_Strings_f(void)
{
    GLint num_extensions, i;

    ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
    ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
    ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string);

    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (i = 0; i < num_extensions; i++)
    {
        ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n",
                      (const char *)glGetStringi(GL_EXTENSIONS, i));
    }
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState(void)
{
    glClearColor(1, 0, 0.5, 0.5);
    glCullFace(GL_FRONT);

    RB_AlphaTest(true);
    RB_Textured(true);
    RB_Fog(false, 0, 1);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    GL_TextureMode(gl_texturemode->string);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval(void)
{
    if (gl_swapinterval->modified)
    {
        gl_swapinterval->modified = false;

#ifdef _WIN32
        if (wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(gl_swapinterval->value);
        }
#endif
    }
}
