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

// draw.cpp

#include "gl_local.h"

image_t *draw_chars;

extern qboolean scrap_dirty;
void Scrap_Upload(void);

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal(void)
{
    // The console font is the sole linearly filtered UI texture.
    draw_chars = GL_FindImage("pics/conchars.pcx", it_pic);
    GL_Bind(draw_chars->texnum);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char(int x, int y, int num)
{
    int row, col;
    float frow, fcol, size;

    num &= 255;

    if ((num & 127) == 32)
    {
        return; // space
    }

    if (y <= -8)
    {
        return; // totally off screen
    }

    row = num >> 4;
    col = num & 15;

    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;

    GL_Bind(draw_chars->texnum);

    RB_Begin(GL_QUADS);
    RB_TexCoord2f(fcol, frow);
    RB_Vertex2f(x, y);
    RB_TexCoord2f(fcol + size, frow);
    RB_Vertex2f(x + 8, y);
    RB_TexCoord2f(fcol + size, frow + size);
    RB_Vertex2f(x + 8, y + 8);
    RB_TexCoord2f(fcol, frow + size);
    RB_Vertex2f(x, y + 8);
    RB_End();
}

/*
=============
Draw_FindPic
=============
*/
image_t *Draw_FindPic(char *name)
{
    image_t *gl;
    char fullname[MAX_QPATH];

    if (name[0] != '/' && name[0] != '\\')
    {
        Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
        gl = GL_FindImage(fullname, it_pic);
    }
    else
    {
        gl = GL_FindImage(name + 1, it_pic);
    }

    return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize(int *w, int *h, char *pic)
{
    image_t *gl;

    gl = Draw_FindPic(pic);
    if (!gl)
    {
        *w = *h = -1;
        return;
    }
    *w = gl->width;
    *h = gl->height;
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic(int x, int y, int w, int h, char *pic)
{
    image_t *gl;

    gl = Draw_FindPic(pic);
    if (!gl)
    {
        ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
        return;
    }

    if (scrap_dirty)
    {
        Scrap_Upload();
    }

    GL_Bind(gl->texnum);
    RB_Begin(GL_QUADS);
    RB_TexCoord2f(gl->sl, gl->tl);
    RB_Vertex2f(x, y);
    RB_TexCoord2f(gl->sh, gl->tl);
    RB_Vertex2f(x + w, y);
    RB_TexCoord2f(gl->sh, gl->th);
    RB_Vertex2f(x + w, y + h);
    RB_TexCoord2f(gl->sl, gl->th);
    RB_Vertex2f(x, y + h);
    RB_End();
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, char *pic)
{
    image_t *gl;

    gl = Draw_FindPic(pic);
    if (!gl)
    {
        ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
        return;
    }
    if (scrap_dirty)
    {
        Scrap_Upload();
    }

    GL_Bind(gl->texnum);
    RB_Begin(GL_QUADS);
    RB_TexCoord2f(gl->sl, gl->tl);
    RB_Vertex2f(x, y);
    RB_TexCoord2f(gl->sh, gl->tl);
    RB_Vertex2f(x + gl->width, y);
    RB_TexCoord2f(gl->sh, gl->th);
    RB_Vertex2f(x + gl->width, y + gl->height);
    RB_TexCoord2f(gl->sl, gl->th);
    RB_Vertex2f(x, y + gl->height);
    RB_End();
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h, char *pic)
{
    image_t *image;

    image = Draw_FindPic(pic);
    if (!image)
    {
        ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
        return;
    }

    GL_Bind(image->texnum);
    RB_Begin(GL_QUADS);
    RB_TexCoord2f(x / 64.0, y / 64.0);
    RB_Vertex2f(x, y);
    RB_TexCoord2f((x + w) / 64.0, y / 64.0);
    RB_Vertex2f(x + w, y);
    RB_TexCoord2f((x + w) / 64.0, (y + h) / 64.0);
    RB_Vertex2f(x + w, y + h);
    RB_TexCoord2f(x / 64.0, (y + h) / 64.0);
    RB_Vertex2f(x, y + h);
    RB_End();
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c)
{
    union
    {
        unsigned c;
        byte v[4];
    } color;

    if ((unsigned)c > 255)
    {
        ri.Sys_Error(ERR_FATAL, "Draw_Fill: bad color");
    }

    RB_Textured(false);

    color.c = d_8to24table[c];
    RB_Color3f(color.v[0] / 255.0, color.v[1] / 255.0, color.v[2] / 255.0);

    RB_Begin(GL_QUADS);

    RB_Vertex2f(x, y);
    RB_Vertex2f(x + w, y);
    RB_Vertex2f(x + w, y + h);
    RB_Vertex2f(x, y + h);

    RB_End();
    RB_Color4f(1, 1, 1, 1);
    RB_Textured(true);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void)
{
    glEnable(GL_BLEND);
    RB_Textured(false);
    RB_Color4f(0, 0, 0, 0.8);
    RB_Begin(GL_QUADS);

    RB_Vertex2f(0, 0);
    RB_Vertex2f(vid.width, 0);
    RB_Vertex2f(vid.width, vid.height);
    RB_Vertex2f(0, vid.height);

    RB_End();
    RB_Color4f(1, 1, 1, 1);
    RB_Textured(true);
    glDisable(GL_BLEND);
}

//====================================================================

/*
=============
Draw_StretchRaw
=============
*/
extern unsigned r_rawpalette[256];

void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data)
{
    unsigned image32[256 * 256];
    int i, j, trows;
    byte *source;
    int frac, fracstep;
    float hscale;
    int row;
    float t;

    GL_Bind(TEXNUM_CINEMATIC);

    if (rows <= 256)
    {
        hscale = 1;
        trows = rows;
    }
    else
    {
        hscale = rows / 256.0;
        trows = 256;
    }
    t = rows * hscale / 256;

    {
        unsigned *dest;

        for (i = 0; i < trows; i++)
        {
            row = (int)(i * hscale);
            if (row > rows)
            {
                break;
            }
            source = data + cols * row;
            dest = &image32[i * 256];
            fracstep = cols * 0x10000 / 256;
            frac = fracstep >> 1;
            for (j = 0; j < 256; j++)
            {
                dest[j] = r_rawpalette[source[frac >> 16]];
                frac += fracstep;
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
    }
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    RB_Begin(GL_QUADS);
    RB_TexCoord2f(0, 0);
    RB_Vertex2f(x, y);
    RB_TexCoord2f(1, 0);
    RB_Vertex2f(x + w, y);
    RB_TexCoord2f(1, t);
    RB_Vertex2f(x + w, y + h);
    RB_TexCoord2f(0, t);
    RB_Vertex2f(x, y + h);
    RB_End();
}

/*
=============
Draw_StretchRaw32

Direct RGBA upload for the RoQ menu cinematics: cols and rows are the
decoded video size, the quad stretches it over x,y,w,h. The core
profile takes non power of two sizes, so the video uploads at its
native size.
=============
*/
void Draw_StretchRaw32(int x, int y, int w, int h, int cols, int rows, byte *data)
{
    GL_Bind(TEXNUM_CINEMATIC);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    RB_Begin(GL_QUADS);
    RB_TexCoord2f(0, 0);
    RB_Vertex2f(x, y);
    RB_TexCoord2f(1, 0);
    RB_Vertex2f(x + w, y);
    RB_TexCoord2f(1, 1);
    RB_Vertex2f(x + w, y + h);
    RB_TexCoord2f(0, 1);
    RB_Vertex2f(x, y + h);
    RB_End();
}
