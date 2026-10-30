/* Original Duke status bar and indexed HUD artwork in the existing screen path. */
#include "client.h"
#include <ctype.h>

static struct
{
    int16_t width, height;
    uint32_t animation;
} tileinfo[6144];

static int tiles_loaded;

static int FragBarRows(void)
{
    if (cls.state != ca_active || cl.frame.playerstate.stats[DN_STAT_FORMAT] != DN_PLAYER_FORMAT ||
        !cl.configstrings[DN_CS_DEATHMATCH][0] ||
        Cvar_Get("cl_hudmode", "2", CVAR_ARCHIVE)->value <= 0)
    {
        return 0;
    }
    int rows = 1;
    for (int i = 0; i < DN_SCORE_PLAYERS; i++)
    {
        if (cl.configstrings[DN_CS_PLAYER_SCORES + i][0])
        {
            rows = i / 4 + 1;
        }
    }
    return rows;
}

int DN_FragBarHeight(void)
{
    return viddef.height * FragBarRows() * 8 / 200;
}

static void LoadTiles(void)
{
    if (!tiles_loaded)
    {
        void *data;
        int size = FS_LoadFile("sprites/tiles.dat", &data);
        if (!data || size != sizeof(tileinfo))
        {
            Com_Error(ERR_DROP, "Missing Duke UI metadata");
        }
        memcpy(tileinfo, data, sizeof(tileinfo));
        FS_FreeFile(data);
        tiles_loaded = 1;
    }
}

int DN_HudHeight(void)
{
    return cl.frame.playerstate.stats[DN_STAT_FORMAT] == DN_PLAYER_FORMAT &&
                   Cvar_Get("cl_hudmode", "2", CVAR_ARCHIVE)->value >= 2
               ? viddef.height * 34 / 200
               : 0;
}

static void Tile(float x, float y, int tile, int shade, int palette, int top_left)
{
    char path[MAX_QPATH];
    LoadTiles();
    if (tile < 0 || tile >= 6144 || !tileinfo[tile].width)
    {
        return;
    }
    if (!top_left)
    {
        x -= tileinfo[tile].width / 2 + (int8_t)(tileinfo[tile].animation >> 8);
        y -= tileinfo[tile].height / 2 + (int8_t)(tileinfo[tile].animation >> 16);
    }
    snprintf(path, sizeof(path), "dn/tile%04d_p%d_s%d", tile, palette, shade < 0 ? 0 : shade);
    int left = (int)lroundf(x * viddef.width / 320), top = (int)lroundf(y * viddef.height / 200);
    int right = (int)lroundf((x + tileinfo[tile].width) * viddef.width / 320);
    int bottom = (int)lroundf((y + tileinfo[tile].height) * viddef.height / 200);
    re.DrawStretchPic(left, top, right - left, bottom - top, path);
}

void DN_DrawUiTile(float x, float y, int tile, int shade, int palette, int top_left)
{
    Tile(x, y, tile, shade, palette, top_left);
}

/* scaled UI tile for the menu framework: the item textscale stretches the
   tile in Duke screen units before the screen projection */
void DN_DrawUiTileScaled(float x, float y, int tile, int shade, int palette, int top_left, float scale)
{
    char path[MAX_QPATH];
    int width, height;
    LoadTiles();
    if (tile < 0 || tile >= 6144 || !tileinfo[tile].width)
    {
        return;
    }
    width = (int)(tileinfo[tile].width * scale + 0.5f);
    height = (int)(tileinfo[tile].height * scale + 0.5f);
    if (!top_left)
    {
        x -= width / 2 + (int8_t)(tileinfo[tile].animation >> 8) * scale;
        y -= height / 2 + (int8_t)(tileinfo[tile].animation >> 16) * scale;
    }
    snprintf(path, sizeof(path), "dn/tile%04d_p%d_s%d", tile, palette, shade < 0 ? 0 : shade);
    int left = (int)lroundf(x * viddef.width / 320), top = (int)lroundf(y * viddef.height / 200);
    int right = (int)lroundf((x + width) * viddef.width / 320);
    int bottom = (int)lroundf((y + height) * viddef.height / 200);
    re.DrawStretchPic(left, top, right - left, bottom - top, path);
}

static void Number(int x, int y, int value)
{
    char text[16];
    int width = 0;
    snprintf(text, sizeof(text), "%d", value < 0 ? 0 : value > 999 ? 999 : value);
    for (int i = 0; text[i]; i++)
    {
        width += tileinfo[2472 + text[i] - '0'].width + 1;
    }
    x -= width / 2;
    for (int i = 0; text[i]; i++)
    {
        int tile = 2472 + text[i] - '0';
        Tile(x, y, tile, 0, 0, 1);
        x += tileinfo[tile].width + 1;
    }
}

static void SmallNumber(int x, int y, int value, int shade, int digits)
{
    char text[16];
    value = value<0 ? 0 : value>(digits == 2 ? 99 : 999) ? (digits == 2 ? 99 : 999) : value;
    snprintf(text, sizeof(text), "%d", value);
    int start = x + (digits - (int)strlen(text)) * 4;
    for (int i = 0; text[i]; i++)
    {
        Tile(start + i * 4, y, 3010 + text[i] - '0', shade, 0, 0);
    }
}

static void WeaponRow(int label, int weapon, int x, int y, int maximum, int digits)
{
    const short *stats = cl.frame.playerstate.stats;
    int current = stats[DN_STAT_WEAPON];
    int owned = (stats[DN_STAT_WEAPONS] & (1 << weapon)) != 0;
    int shade = 12 + (owned ? 0 : 9);
    if (weapon == DN_WEAPON_PIPEBOMB && !stats[DN_AmmoStat(weapon)])
    {
        shade = 21;
    }
    if (current == weapon || (weapon == DN_WEAPON_PIPEBOMB && current == DN_WEAPON_REMOTE))
    {
        shade = 0;
    }
    Tile(x - 7, y, 3010 + label, shade > 10 ? shade - 10 : 0, 7, 0);
    Tile(x - (digits == 3 ? 4 : 3), y, 3020, shade, 0, 0);
    Tile(x + (digits == 3 ? 13 : 9), y, 3021, shade, 0, 0);
    SmallNumber(x, y, stats[DN_AmmoStat(weapon)], shade, digits);
    SmallNumber(x + (digits == 3 ? 17 : 13), y, maximum, shade, digits);
}

static void MiniText(int x, int y, const char *text, int palette, int limit)
{
    for (int i = 0; text[i] && i < limit; i++)
    {
        int glyph = toupper((unsigned char)text[i]);
        if (glyph >= '!' && glyph <= '~')
        {
            Tile(x, y, 3072 + glyph - '!', 0, palette, 1);
        }
        x += glyph == ' ' ? 5 : 4;
    }
}

static void FragBar(void)
{
    int rows = FragBarRows();
    for (int row = 0; row < rows; row++)
    {
        Tile(0, row * 8, 2465, 0, 0, 1);
    }
    for (int i = 0; rows && i < DN_SCORE_PLAYERS; i++)
    {
        int palette, frags;
        char name[32], score[16];
        if (sscanf(cl.configstrings[DN_CS_PLAYER_SCORES + i], "%d %d %31[^\n]", &palette, &frags,
                   name) != 3 ||
            palette < 0 || palette > 255)
        {
            continue;
        }
        int x = 21 + 73 * (i & 3), y = 2 + 8 * (i / 4);
        MiniText(x, y, name, palette, 9);
        snprintf(score, sizeof(score), "%d", frags);
        MiniText(x + 46, y, score, palette, 6);
    }
}

void DN_DrawHud(void)
{
    const short *stats = cl.frame.playerstate.stats;
    int mode = (int)Cvar_Get("cl_hudmode", "2", CVAR_ARCHIVE)->value;
    if (!mode || cls.state != ca_active)
    {
        return;
    }
    LoadTiles();
    FragBar();
    if (mode == 1)
    {
        Tile(5, 172, 30, 0, 21, 1);
        Tile(37, 172, 31, 0, 21, 1);
        Number(20, 183, stats[STAT_HEALTH]);
        Number(53, 183, stats[STAT_AMMO]);
    }
    else
    {
        int limits[10] = {200, 50, 200, 50, 50, 50, 99, 10, 99, 50};
        sscanf(cl.configstrings[CS_GENERAL], "%d%d%d%d%d%d%d%d%d%d", &limits[0], &limits[1],
               &limits[2], &limits[3], &limits[4], &limits[5], &limits[6], &limits[7], &limits[8],
               &limits[9]);
        Tile(0, 166, 2462, 4, 0, 1);
        Number(32, 183, stats[STAT_HEALTH]);
        Number(64, 183, stats[STAT_ARMOR]);
        if (stats[DN_STAT_WEAPON] != DN_WEAPON_KNEE)
        {
            Number(208, 183, stats[STAT_AMMO]);
        }
        WeaponRow(2, DN_WEAPON_PISTOL, 96, 182, limits[0], 3);
        WeaponRow(3, DN_WEAPON_SHOTGUN, 96, 188, limits[1], 3);
        WeaponRow(4, DN_WEAPON_CHAINGUN, 96, 194, limits[2], 3);
        WeaponRow(5, DN_WEAPON_RPG, 135, 182, limits[3], 2);
        WeaponRow(6, DN_WEAPON_PIPEBOMB, 135, 188, limits[4], 2);
        int grow = stats[DN_STAT_WEAPON] == DN_WEAPON_GROW;
        WeaponRow(7, grow ? DN_WEAPON_GROW : DN_WEAPON_SHRINKER, 135, 194, limits[grow ? 9 : 5], 2);
        WeaponRow(8, DN_WEAPON_DEVASTATOR, 166, 182, limits[6], 2);
        WeaponRow(9, DN_WEAPON_TRIPBOMB, 166, 188, limits[7], 2);
        WeaponRow(0, DN_WEAPON_FREEZE, 166, 194, limits[8], 2);
        if (stats[DN_STAT_KEYS] & 4)
        {
            Tile(275, 182, 2471, 0, 23, 1);
        }
        if (stats[DN_STAT_KEYS] & 2)
        {
            Tile(288, 182, 2471, 0, 21, 1);
        }
        if (stats[DN_STAT_KEYS] & 1)
        {
            Tile(281, 189, 2471, 0, 0, 1);
        }
    }
    if (stats[DN_STAT_INVENTORY] > 0 && stats[DN_STAT_INVENTORY] <= 7)
    {
        static const int icons[] = {2460, 2469, 2470, 2467, 2461, 2468, 2463};
        int offset = mode == 1 ? 158 : 0;
        if (mode == 1)
        {
            Tile(69, 170, 33, 0, 21, 1);
        }
        Tile(231 - offset, 179, icons[stats[DN_STAT_INVENTORY] - 1], 0, 0, 1);
        SmallNumber(250 - offset, 194, stats[DN_STAT_INVENTORY_AMOUNT], 0, 3);
        Tile(262 - offset, 190, 3072 + '%' - '!', 0, 6, 1);
    }
}
