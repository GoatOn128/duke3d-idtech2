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

int DN_MenuText(int x, int y, const char *text, int selected)
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
            x += 5;
            continue;
        }
        snprintf(path, sizeof(path), "dn/tile%04d_p0_s0", tile);
        re.DrawGetPicSize(&width, &height, path);
        DN_DrawUiTile(x, y - 12, tile, selected ? 0 : 16, 0, 1);
        x += width > 0 ? width : 5;
    }
    return start;
}

void DN_MenuCursor(int x, int y)
{
    DN_DrawUiTile(x, y, 2813 + (cls.realtime / 100) % 7, 0, 0, 0);
}

void DN_MenuBackground(void)
{
    DN_DrawUiTile(0, 0, 2456, 16, 0, 1);
}
