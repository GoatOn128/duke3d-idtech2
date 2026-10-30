/* Duke's original menu glyphs and coordinates within the existing menu stack. */
#include "client.h"
#include <ctype.h>

static int Glyph(unsigned char c)
{
    if (c >= '0' && c <= '9')
    {
        return 2930 + c - '0';
    }
    c = toupper(c);
    if (c >= 'A' && c <= 'Z')
    {
        return 2940 + c - 'A';
    }
    switch (c)
    {
    case '-':
        return 2929;
    case '.':
        return 3002;
    case ',':
        return 3003;
    case '!':
        return 3004;
    case '?':
        return 3005;
    case ';':
        return 3006;
    case ':':
        return 3007;
    case '\'':
        return 3022;
    }
    return -1;
}

/* width of a line in unscaled Duke pixels, the measure the centered
   line math uses */
int DN_MenuTextWidth(const char *text)
{
    int total = 0;
    for (const unsigned char *c = (const unsigned char *)text; *c; c++)
    {
        int tile = Glyph(*c), width, height;
        char path[MAX_QPATH];
        if (tile < 0)
        {
            total += 5;
            continue;
        }
        snprintf(path, sizeof(path), "dn/tile%04d_p0_s0", tile);
        re.DrawGetPicSize(&width, &height, path);
        total += width > 0 ? width - 1 : 5;
    }
    return total;
}

/* shared body: scale 1 draws the original menu glyphs, smaller scales
   shrink each glyph while the line origin stays put */
static int MenuTextScaled(int x, int y, const char *text, int selected, float scale)
{
    int total = (int)(DN_MenuTextWidth(text) * scale);
    if (x == 160)
    {
        x = (320 - total - 10) / 2;
    }
    int start = x;
    for (const unsigned char *c = (const unsigned char *)text; *c; c++)
    {
        int tile = Glyph(*c), width, height;
        char path[MAX_QPATH];
        if (tile < 0)
        {
            x += (int)(5 * scale);
            continue;
        }
        snprintf(path, sizeof(path), "dn/tile%04d_p0_s0", tile);
        re.DrawGetPicSize(&width, &height, path);
        DN_DrawUiTileScaled(x, y - 12, tile, selected ? 0 : 16, 0, 1, scale);
        x += (int)((width > 0 ? width : 5) * scale);
    }
    return start;
}

int DN_MenuText(int x, int y, const char *text, int selected)
{
    return MenuTextScaled(x, y, text, selected, 1.0f);
}

int DN_MenuTextScaled(int x, int y, const char *text, int selected, float scale)
{
    return MenuTextScaled(x, y, text, selected, scale);
}

void DN_MenuCursor(int x, int y)
{
    DN_DrawUiTile(x, y, 2813 + (cls.realtime / 100) % 7, 0, 0, 0);
}

void DN_MenuBackground(void)
{
    DN_DrawUiTile(0, 0, 2456, 16, 0, 1);
}
