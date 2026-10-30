/* Client side RoQ cinematic player for the UI module. A windowDef with the
   cinematic keyword plays an .roq inside its rect: the display context
   playCinematic/stopCinematic/drawCinematic/runCinematicFrame hooks hand
   the stream to this file, the frames decode into RGBA and the renderer
   uploads them through DrawStretchRaw32. The chunk layout follows the
   public RoQ format description (idroq.txt, Dr. Tim Ferguson, 2001); the
   code is written from scratch for this project, no third-party code is
   included. */

#include "client.h"

#define ROQ_MAX_HANDLES 8
#define ROQ_FPS 30

/* chunk ids */
#define ROQ_CHUNK_INFO 0x1001
#define ROQ_CHUNK_CODEBOOK 0x1002
#define ROQ_CHUNK_QUADVQ 0x1011
#define ROQ_CHUNK_SOUND_MONO 0x1020
#define ROQ_CHUNK_SOUND_STEREO 0x1021

/* block codes inside a QUAD_VQ payload */
#define ROQ_CODE_MOT 0
#define ROQ_CODE_FCC 1
#define ROQ_CODE_SLD 2
#define ROQ_CODE_CCC 3

typedef struct
{
    byte *data; /* the whole file, owned by FS_LoadFile */
    int size;
    int pos; /* stream position, past the signature chunk */
    int width, height;
    byte *current; /* frame being decoded */
    byte *last; /* the previous decoded frame, the draw and motion source */
    byte cells2x2[256][6]; /* 2x2 YCbCr cells: Y0 Y1 Y2 Y3 Cb Cr */
    byte cells4x4[256][4]; /* 4x4 cells: indices into cells2x2 */
    int startTime;
    int frames;
    qboolean finished; /* the stream ran out */
    qboolean finishReported; /* the one shot end report to the UI */
} roq_t;

static roq_t s_roq[ROQ_MAX_HANDLES];

static unsigned RoQ_Read16(const byte *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned RoQ_Read32(const byte *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) |
           ((unsigned)p[3] << 24);
}

/* BT.601 YCbCr to RGB for one pixel; the codebooks store full range luma
   with chroma centered on 128 */
static void RoQ_WritePixel(byte *out, int y, int cb, int cr)
{
    int r, g, b;

    cb -= 128;
    cr -= 128;
    r = y + (cr * 1436) / 1024;
    g = y - (cb * 352) / 1024 - (cr * 731) / 1024;
    b = y + (cb * 1815) / 1024;
    out[0] = (byte)(r < 0 ? 0 : r > 255 ? 255 : r);
    out[1] = (byte)(g < 0 ? 0 : g > 255 ? 255 : g);
    out[2] = (byte)(b < 0 ? 0 : b > 255 ? 255 : b);
    out[3] = 255;
}

/* writes one 2x2 codebook cell at x,y */
static void RoQ_PutCell2x2(roq_t *roq, int x, int y, const byte *cell)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        int px = x + (i & 1);
        int py = y + (i >> 1);

        if (px < 0 || px >= roq->width || py < 0 || py >= roq->height)
        {
            continue;
        }
        RoQ_WritePixel(roq->current + (py * roq->width + px) * 4, cell[i], cell[4], cell[5]);
    }
}

/* writes one 2x2 codebook cell doubled to 4x4 at x,y */
static void RoQ_PutCell4x4(roq_t *roq, int x, int y, const byte *cell)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        int px = x + (i & 3);
        int py = y + (i >> 2);

        if (px < 0 || px >= roq->width || py < 0 || py >= roq->height)
        {
            continue;
        }
        /* local pixel (cx,cy) takes cell index ((cy >> 1) * 2 + (cx >> 1)) */
        RoQ_WritePixel(roq->current + (py * roq->width + px) * 4,
                       cell[((i >> 3) << 1) + ((i >> 1) & 1)], cell[4], cell[5]);
    }
}

/* copies a block from the previous frame with the source clamped at the
   frame edges, the motion behavior the reference players show */
static void RoQ_CopyBlock(roq_t *roq, int x, int y, int fromx, int fromy, int w, int h)
{
    int sx, sy;

    for (sy = 0; sy < h; sy++)
    {
        for (sx = 0; sx < w; sx++)
        {
            int fx = fromx + sx;
            int fy = fromy + sy;

            if (x + sx < 0 || x + sx >= roq->width || y + sy < 0 || y + sy >= roq->height)
            {
                continue;
            }
            if (fx < 0)
            {
                fx = 0;
            }
            else if (fx >= roq->width)
            {
                fx = roq->width - 1;
            }
            if (fy < 0)
            {
                fy = 0;
            }
            else if (fy >= roq->height)
            {
                fy = roq->height - 1;
            }
            memcpy(roq->current + ((y + sy) * roq->width + (x + sx)) * 4,
                   roq->last + (fy * roq->width + fx) * 4, 4);
        }
    }
}

/* reads the next 2 bit block code; the flag words arrive lazily from the
   payload stream, eight codes per word, first code in the top bits */
static int RoQ_NextCode(const byte *payload, int size, int *bpos, int *vqflg, int *vqflgPos)
{
    if (*vqflgPos < 0)
    {
        if (*bpos + 2 > size)
        {
            return -1;
        }
        *vqflg = (int)RoQ_Read16(payload + *bpos);
        *bpos += 2;
        *vqflgPos = 7;
    }
    (*vqflgPos)--;
    return (*vqflg >> ((*vqflgPos + 1) * 2)) & 0x3;
}

/* decodes one QUAD_VQ payload: 16x16 macroblocks in raster order, four 8x8
   quadrants each, the data bytes follow the code that consumes them */
static void RoQ_DecodeQuads(roq_t *roq, int chunkSize, int chunkArg)
{
    const byte *payload = roq->data + roq->pos;
    int bpos = 0;
    int vqflg = 0, vqflgPos = -1;
    int mx, my;

    for (my = 0; my < roq->height; my += 16)
    {
        for (mx = 0; mx < roq->width; mx += 16)
        {
            int qx, qy;

            for (qy = 0; qy < 16; qy += 8)
            {
                for (qx = 0; qx < 16; qx += 8)
                {
                    int vqid = RoQ_NextCode(payload, chunkSize, &bpos, &vqflg, &vqflgPos);
                    int xp = mx + qx, yp = my + qy;

                    if (vqid < 0)
                    {
                        return;
                    }
                    if (vqid == ROQ_CODE_MOT)
                    {
                        continue;
                    }
                    if (bpos >= chunkSize)
                    {
                        return;
                    }
                    if (vqid == ROQ_CODE_FCC)
                    {
                        int b = payload[bpos++];

                        /* the chunk argument carries the mean motion the
                           byte codes against */
                        RoQ_CopyBlock(roq, xp, yp,
                                      xp + 8 - (b >> 4) - (signed char)(chunkArg >> 8),
                                      yp + 8 - (b & 0xf) - (signed char)(chunkArg & 0xff), 8, 8);
                        continue;
                    }
                    if (vqid == ROQ_CODE_SLD)
                    {
                        int qcell = payload[bpos++];

                        RoQ_PutCell4x4(roq, xp, yp, roq->cells2x2[roq->cells4x4[qcell][0]]);
                        RoQ_PutCell4x4(roq, xp + 4, yp, roq->cells2x2[roq->cells4x4[qcell][1]]);
                        RoQ_PutCell4x4(roq, xp, yp + 4, roq->cells2x2[roq->cells4x4[qcell][2]]);
                        RoQ_PutCell4x4(roq, xp + 4, yp + 4, roq->cells2x2[roq->cells4x4[qcell][3]]);
                        continue;
                    }
                    /* ROQ_CODE_CCC: four coded 4x4 blocks */
                    {
                        int k;

                        for (k = 0; k < 4; k++)
                        {
                            int x = xp + (k & 1) * 4;
                            int y = yp + (k >> 1) * 4;
                            int subid = RoQ_NextCode(payload, chunkSize, &bpos, &vqflg, &vqflgPos);

                            if (subid < 0)
                            {
                                return;
                            }
                            if (subid == ROQ_CODE_MOT)
                            {
                                continue;
                            }
                            if (bpos >= chunkSize)
                            {
                                return;
                            }
                            if (subid == ROQ_CODE_FCC)
                            {
                                int b = payload[bpos++];

                                RoQ_CopyBlock(roq, x, y,
                                              x + 8 - (b >> 4) - (signed char)(chunkArg >> 8),
                                              y + 8 - (b & 0xf) - (signed char)(chunkArg & 0xff), 4, 4);
                            }
                            else if (subid == ROQ_CODE_SLD)
                            {
                                int qcell = payload[bpos++];

                                RoQ_PutCell2x2(roq, x, y, roq->cells2x2[roq->cells4x4[qcell][0]]);
                                RoQ_PutCell2x2(roq, x + 2, y, roq->cells2x2[roq->cells4x4[qcell][1]]);
                                RoQ_PutCell2x2(roq, x, y + 2, roq->cells2x2[roq->cells4x4[qcell][2]]);
                                RoQ_PutCell2x2(roq, x + 2, y + 2, roq->cells2x2[roq->cells4x4[qcell][3]]);
                            }
                            else
                            {
                                /* four direct 2x2 cells */
                                int c;

                                if (bpos + 4 > chunkSize)
                                {
                                    return;
                                }
                                for (c = 0; c < 4; c++)
                                {
                                    RoQ_PutCell2x2(roq, x + (c & 1) * 2, y + (c >> 1) * 2,
                                                   roq->cells2x2[payload[bpos + c]]);
                                }
                                bpos += 4;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* reads the 2x2 and 4x4 codebooks; a count byte of zero means 256 entries */
static void RoQ_ReadCodebook(roq_t *roq, int chunkSize, int chunkArg)
{
    int nv1 = (chunkArg >> 8) & 0xff;
    int nv2 = chunkArg & 0xff;
    int pos = 0, i;

    if (nv1 == 0)
    {
        nv1 = 256;
    }
    if (nv2 == 0 && nv1 * 6 < chunkSize)
    {
        nv2 = 256;
    }
    if (nv1 * 6 + nv2 * 4 > chunkSize)
    {
        /* a broken codebook leaves the previous one in place rather than
           reading past the chunk */
        return;
    }
    for (i = 0; i < nv1; i++)
    {
        memcpy(roq->cells2x2[i], roq->data + roq->pos + pos, 6);
        pos += 6;
    }
    for (i = 0; i < nv2; i++)
    {
        memcpy(roq->cells4x4[i], roq->data + roq->pos + pos, 4);
        pos += 4;
    }
}

/* walks chunks until one video frame decoded; sound chunks arrive
   interleaved and are skipped, the Duke cinematics carry no audio */
static void RoQ_ReadFrame(roq_t *roq)
{
    while (roq->pos + 8 <= roq->size)
    {
        unsigned id = RoQ_Read16(roq->data + roq->pos);
        unsigned size = RoQ_Read32(roq->data + roq->pos + 2);
        unsigned arg = RoQ_Read16(roq->data + roq->pos + 6);

        if ((unsigned long)roq->pos + 8 + size > (unsigned long)roq->size)
        {
            roq->finished = true;
            return;
        }
        roq->pos += 8;

        if (id == ROQ_CHUNK_QUADVQ)
        {
            byte *t;

            if (!roq->width)
            {
                roq->finished = true;
                return;
            }
            RoQ_DecodeQuads(roq, (int)size, (int)arg);
            roq->pos += size;
            /* double buffer swap: the frame just decoded becomes the draw
               and motion source for the next one */
            t = roq->current;
            roq->current = roq->last;
            roq->last = t;
            roq->frames++;
            return;
        }
        if (id == ROQ_CHUNK_INFO)
        {
            if (!roq->width && size >= 8)
            {
                int w = (int)RoQ_Read16(roq->data + roq->pos);
                int h = (int)RoQ_Read16(roq->data + roq->pos + 2);

                /* the quad walk covers the frame in 16x16 macroblocks */
                if (w > 0 && h > 0 && w <= 2048 && h <= 2048 && (w % 16) == 0 && (h % 16) == 0)
                {
                    roq->width = w;
                    roq->height = h;
                    roq->current = (byte *)malloc((size_t)w * h * 4);
                    roq->last = (byte *)malloc((size_t)w * h * 4);
                    if (!roq->current || !roq->last)
                    {
                        roq->width = 0;
                        roq->finished = true;
                        return;
                    }
                    memset(roq->current, 0, (size_t)w * h * 4);
                    memset(roq->last, 0, (size_t)w * h * 4);
                }
                else
                {
                    roq->finished = true;
                    return;
                }
            }
        }
        else if (id == ROQ_CHUNK_CODEBOOK)
        {
            RoQ_ReadCodebook(roq, (int)size, (int)arg);
        }
        /* sound chunks and unknown ids skip */
        roq->pos += size;
    }
    roq->finished = true;
}

static roq_t *RoQ_ForHandle(int handle)
{
    if (handle < 0 || handle >= ROQ_MAX_HANDLES || !s_roq[handle].data)
    {
        return NULL;
    }
    return &s_roq[handle];
}

int DN_ROQ_PlayCinematic(const char *name, float x, float y, float w, float h)
{
    byte *data = NULL;
    int size, i;

    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!name || !name[0])
    {
        return -1;
    }
    for (i = 0; i < ROQ_MAX_HANDLES; i++)
    {
        if (!s_roq[i].data)
        {
            break;
        }
    }
    if (i == ROQ_MAX_HANDLES)
    {
        return -1;
    }
    size = FS_LoadFile((char *)name, (void **)&data);
    if (!data || size < 16)
    {
        if (data)
        {
            FS_FreeFile(data);
        }
        return -1;
    }
    /* the signature chunk: id 0x1084, size -1, the frame rate in the
       argument */
    if (RoQ_Read16(data) != 0x1084 || RoQ_Read32(data + 2) != 0xffffffff)
    {
        Com_Printf("RoQ: %s is not a RoQ stream\n", name);
        FS_FreeFile(data);
        return -1;
    }
    memset(&s_roq[i], 0, sizeof(s_roq[i]));
    s_roq[i].data = data;
    s_roq[i].size = size;
    s_roq[i].pos = 8; /* past the signature chunk */
    s_roq[i].startTime = cls.realtime;
    return i;
}

void DN_ROQ_StopCinematic(int handle)
{
    roq_t *roq = RoQ_ForHandle(handle);

    if (!roq)
    {
        return;
    }
    FS_FreeFile(roq->data);
    free(roq->current);
    free(roq->last);
    memset(roq, 0, sizeof(*roq));
}

void DN_ROQ_StopAll(void)
{
    int i;

    for (i = 0; i < ROQ_MAX_HANDLES; i++)
    {
        DN_ROQ_StopCinematic(i);
    }
}

int DN_ROQ_RunCinematicFrame(int handle)
{
    roq_t *roq = RoQ_ForHandle(handle);

    if (!roq)
    {
        return 0;
    }
    if (!roq->finished)
    {
        /* decode at the fixed stream rate, catching up when the menu
           stalls the frame loop */
        int target = (cls.realtime - roq->startTime) * ROQ_FPS / 1000 + 1;

        while (roq->frames < target && !roq->finished)
        {
            RoQ_ReadFrame(roq);
        }
    }
    /* the stream reports its end exactly once, then the last frame keeps
       drawing until the window stops the cinematic */
    if (roq->finished && !roq->finishReported)
    {
        roq->finishReported = true;
        return 1;
    }
    return 0;
}

void DN_ROQ_DrawCinematic(int handle, float x, float y, float w, float h)
{
    roq_t *roq = RoQ_ForHandle(handle);

    if (!roq || !roq->frames || viddef.width <= 0 || viddef.height <= 0)
    {
        return;
    }
    /* the rect arrives in the 640x480 virtual menu space; the video
       stretches into it at the current video mode */
    re.DrawStretchRaw32((int)(x * viddef.width / 640.0f),
                        (int)(y * viddef.height / 480.0f),
                        (int)(w * viddef.width / 640.0f + 0.5f),
                        (int)(h * viddef.height / 480.0f + 0.5f),
                        roq->width, roq->height, roq->last);
}
