/* Original bonus-screen artwork and staged results in the normal screen path. */
#include "client.h"
#include <ctype.h>

static int started, last_stage;
static qboolean showing, revealed;

static qboolean Active(void)
{
    return cls.state == ca_active &&
           cl.frame.playerstate.stats[DN_STAT_FORMAT] == DN_PLAYER_FORMAT &&
           cl.frame.playerstate.stats[DN_STAT_INTERMISSION];
}

static void Text(int x, int y, const char *text, int shade)
{
    int total = 0;
    for (const unsigned char *c = (const unsigned char *)text; *c; c++)
    {
        int width = 0, height;
        char path[MAX_QPATH];
        int ch = toupper(*c);
        if (ch >= '!' && ch <= '~')
        {
            snprintf(path, sizeof(path), "dn/tile%04d_p0_s0", 2822 + ch - '!');
            re.DrawGetPicSize(&width, &height, path);
        }
        total += ch >= '0' && ch <= '9' ? 8 : width > 0 ? width : 5;
    }
    if (x == 160)
    {
        x = (320 - total) / 2;
    }
    for (const unsigned char *c = (const unsigned char *)text; *c; c++)
    {
        int width = 0, height;
        char path[MAX_QPATH];
        int ch = toupper(*c), tile = 2822 + ch - '!';
        if (ch >= '!' && ch <= '~')
        {
            snprintf(path, sizeof(path), "dn/tile%04d_p0_s0", tile);
            re.DrawGetPicSize(&width, &height, path);
            if (width > 0)
            {
                DN_DrawUiTile(x, y, tile, shade, 0, 1);
            }
        }
        x += ch >= '0' && ch <= '9' ? 8 : width > 0 ? width : 5;
    }
}

static void Time(int y, int seconds)
{
    char text[32];
    seconds = seconds < 0 ? 0 : seconds;
    snprintf(text, sizeof(text), "%02d:%02d", seconds / 60, seconds % 60);
    Text(151, y, text, 0);
}

static void Count(int y, int count)
{
    char text[32];
    snprintf(text, sizeof(text), "%d", count < 0 ? 0 : count);
    Text(150, y, text, 0);
}

qboolean DN_IntermissionKey(int key)
{
    if (!Active() || cls.key_dest != key_game || key == K_F12)
    {
        return false;
    }
    if (!showing || cl.time - started < 1000)
    {
        return true;
    }
    if (!revealed && cl.time - started < 6500)
    {
        revealed = true;
    }
    else
    {
        Cbuf_AddText("cmd dn_continue\n");
    }
    return true;
}

qboolean DN_DrawIntermission(void)
{
    int episode, seconds, par, designer, enemies, secrets;
    if (!Active())
    {
        showing = false;
        return false;
    }
    const char *info = cl.configstrings[DN_CS_INTERMISSION];
    if (sscanf(info, "%d %d %d %d %d %d", &episode, &seconds, &par, &designer, &enemies,
               &secrets) != 6)
    {
        return false;
    }
    if (!showing)
    {
        showing = true;
        revealed = false;
        started = cl.time;
        last_stage = 0;
    }
    int elapsed = revealed ? 6500 : cl.time - started;
    int base = episode == 2 ? 3245 : 3240;
    re.DrawFill(0, 0, viddef.width, viddef.height, 0);
    DN_DrawUiTile(0, 0, base, 0, 0, 1);
    int frame =
        (((cl.time - started) / 1000 * 120 + (cl.time - started) % 1000 * 120 / 1000) >> 5) & 3;
    if (frame)
    {
        DN_DrawUiTile(199, 31, base + (frame == 2 ? 2 : 1), 0, 0, 1);
    }
    DN_MenuText(160, 14, cl.configstrings[DN_CS_LEVEL_NAME], 1);
    DN_MenuText(160, 30, "COMPLETED", 1);
    Text(160, 192, "PRESS ANY KEY TO CONTINUE", 16);
    if (elapsed > 1500)
    {
        Text(10, 68, "Your Time:", 0);
        Text(10, 78, "Par time:", 0);
        Text(10, 87, "3D Realms' Time:", 0);
    }
    if (elapsed > 2000)
    {
        Time(69, seconds);
        Time(78, par);
        Time(87, designer);
    }
    if (elapsed > 3000)
    {
        Text(10, 103, "Enemies Killed:", 0);
        Text(10, 112, "Enemies Left:", 0);
    }
    if (elapsed > 3500)
    {
        int kills = cl.frame.playerstate.stats[STAT_FRAGS];
        Count(102, kills);
        if (enemies < 0)
        {
            Text(150, 112, "N/A", 0);
        }
        else
        {
            Count(112, enemies - kills);
        }
    }
    if (elapsed > 4500)
    {
        Text(10, 129, "Secrets Found:", 0);
        Text(10, 139, "Secrets Missed:", 0);
    }
    if (elapsed > 5000)
    {
        int found = cl.frame.playerstate.stats[DN_STAT_SECRETS];
        Count(129, found);
        Count(139, secrets - found);
    }
    int stage = elapsed > 5000 ? 3 : elapsed > 3500 ? 2 : elapsed > 2000 ? 1 : 0;
    if (stage > last_stage)
    {
        S_StartLocalSound("bombexpl.wav");
        last_stage = stage;
    }
    return true;
}
