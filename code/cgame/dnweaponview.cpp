/* Weapon composition adapted from 3D Realms' GPL PLAYER.cpp displayweapon.
   Coordinates retain the original 320x200 rotatesprite anchors. */
#include "../game/q_shared.h"
#include "dnweaponview.h"

static int Sine(int angle)
{
    return (int)lroundf(sinf((angle & 2047) * (float)M_PI / 1024) * 16384);
}

static void Sprite(dn_weapon_sprites_t *result, int x, int y, int tile, int shade, int palette,
                   int mirror)
{
    if (result->count < sizeof(result->sprites) / sizeof(result->sprites[0]))
    {
        result->sprites[result->count++] = (dn_weapon_sprite_t){tile, x, y, shade, palette, mirror};
    }
}

void DN_BuildWeaponView(const dn_weapon_view_t *v, dn_weapon_sprites_t *out)
{
    int k = v->phase, base = v->base, shade = v->shade > 24 ? 24 : v->shade, pal = v->palette;
    int x = 12 - Sine((v->sway >> 1) + 512) / 1536;
    int gun = 80 - v->position * v->position - abs(Sine(v->sway >> 1) >> 10);
    int arc = 0;
    memset(out, 0, sizeof(*out));
    if (v->weapon < 0 || v->weapon >= DN_WEAPON_COUNT || k < 0 || k > 31)
    {
        return;
    }
    if (v->access > 0 && v->access <= 20)
    {
        static const int y[] = {0,    -8,   -16,  -32,  -64,  -84, -108, -108, -108, -108, -108,
                                -108, -108, -108, -108, -108, -96, -72,  -64,  -32,  -16};
        int frame = v->access;
        Sprite(out, 170 + (y[frame] >> 2), y[frame] + 260, frame >= 11 ? 2563 + (frame >> 3) : 2568,
               shade, 0, frame < 11);
        return;
    }
    switch (v->weapon)
    {
    case DN_WEAPON_KNEE:
        if (k)
        {
            Sprite(out, x + (k < 5 || k > 9 ? 220 : 160), (k < 5 || k > 9 ? 250 : 214) - gun,
                   base + !(k < 5 || k > 9), shade, pal, 0);
        }
        break;
    case DN_WEAPON_PISTOL:
        if (k < 5)
        {
            static const int frames[] = {0, 1, 2, 0, 0};
            Sprite(out, 183 + x - (k == 2 ? 3 : 0), 244 - gun, base + frames[k], shade, pal, 0);
        }
        else if (k < 10)
        {
            Sprite(out, 194, 230 - gun, base + 4, shade, pal, 0);
        }
        else if (k < 15)
        {
            Sprite(out, 244 - k * 8, 130 - gun + k * 16, base + 6, shade, pal, 0);
            Sprite(out, 224, 220 - gun, base + 5, shade, pal, 0);
        }
        else if (k < 20)
        {
            Sprite(out, 124 + k * 2, 430 - gun - k * 8, base + 6, shade, pal, 0);
            Sprite(out, 224, 220 - gun, base + 5, shade, pal, 0);
        }
        else if (k < 23)
        {
            Sprite(out, 184, 235 - gun, base + 8, shade, pal, 0);
            Sprite(out, 224, 210 - gun, base + 5, shade, pal, 0);
        }
        else if (k < 25)
        {
            Sprite(out, 164, 245 - gun, base + 8, shade, pal, 0);
            Sprite(out, 224, 220 - gun, base + 5, shade, pal, 0);
        }
        else if (k < 27)
        {
            Sprite(out, 194, 235 - gun, base + 5, shade, pal, 0);
        }
        break;
    case DN_WEAPON_SHOTGUN:
        x -= 8;
        if (k <= 2 || (k >= 6 && k <= 8))
        {
            if (k == 1 || k == 2)
            {
                Sprite(out, x + 168, 201 - gun, base + 2, -128, pal, 0);
            }
            Sprite(out, x + 146, 202 - gun, base, shade, pal, 0);
        }
        else if (k <= 12)
        {
            if (k > 1 && k < 5)
            {
                gun -= 40;
                x += 20;
                Sprite(out, x + 178, 194 - gun, base + 1 + ((k - 1) >> 1), -128, pal, 0);
            }
            Sprite(out, x + 158, 220 - gun, base + 3, shade, pal, 0);
        }
        else if (k <= 15)
        {
            Sprite(out, x + 198, 210 - gun, base + 4, shade, pal, 0);
        }
        else if (k <= 19 || (k >= 24 && k <= 27))
        {
            Sprite(out, x + 234, 196 - gun, base + 5, shade, pal, 0);
        }
        else if (k <= 23)
        {
            Sprite(out, x + 240, 196 - gun, base + 6, shade, pal, 0);
        }
        else if (k <= 30)
        {
            Sprite(out, x + 188, 206 - gun, base + 4, shade, pal, 0);
        }
        break;
    case DN_WEAPON_CHAINGUN:
        if (k)
        {
            gun -= Sine(k << 7) >> 12;
        }
        Sprite(out, x + 168, 260 - gun, base, shade, pal, 0);
        if (k > 4 && k < 12)
        {
            Sprite(out, x + 136, 208 - (k >> 1) - gun, base + 5 + (k - 4) / 5, -32, pal, 0);
            Sprite(out, x + 180, 208 - (k >> 1) - gun, base + 5 + (k - 4) / 5, -32, pal, 0);
        }
        if (k && k < 8)
        {
            Sprite(out, x + 158, 208 - (k >> 1) - gun, base + 5 + (k - 2) / 5, -32, pal, 0);
        }
        Sprite(out, x + 178, 233 - gun, base + 1 + (k && k < 8 ? k >> 1 : 0), shade, pal, 0);
        break;
    case DN_WEAPON_RPG:
        x -= Sine(768 + (k << 7)) >> 11;
        gun += Sine(768 + (k << 7)) >> 11;
        if (k > 0 && k < 8)
        {
            Sprite(out, x + 164, 176 - gun, base + (k >> 1), shade, pal, 0);
        }
        Sprite(out, x + 164, 176 - gun, base, shade, pal, 0);
        break;
    case DN_WEAPON_PIPEBOMB:
        if (k > 0 && k <= 20)
        {
            if (k < 7)
            {
                gun -= 10 * k;
            }
            else if (k < 12)
            {
                gun += 20 * (k - 10);
            }
            else if (k < 20)
            {
                gun -= 9 * (k - 14);
            }
            Sprite(out, x + 190, 250 - gun, base + (k < 5 ? 0 : k < 12 ? 1 : 2), shade, pal, 0);
        }
        else
        {
            Sprite(out, x + 190, 260 - gun, base, shade, pal, 0);
        }
        break;
    case DN_WEAPON_REMOTE:
    {
        static const int frames[] = {0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0};
        Sprite(out, 102, 258 - gun, base + frames[k <= 10 ? k : 0], shade, pal, 0);
        break;
    }
    case DN_WEAPON_DEVASTATOR:
    {
        static const int motion[] = {0, 4, 12, 24, 12, 4, 0};
        int d = motion[k <= 6 ? k : 0];
        Sprite(out, x + 268 + (v->side ? d / 2 : 0), 238 - gun + (v->side ? d : 0),
               base + (v->side && k >= 4), v->side && k ? -32 : shade, pal, 0);
        Sprite(out, x + 30 - (!v->side ? d / 2 : 0), 240 - gun + (!v->side ? d : 0),
               base + (!v->side && k >= 4), !v->side && k ? -32 : shade, pal, 1);
        break;
    }
    case DN_WEAPON_FREEZE:
        if (k)
        {
            static const int frames[] = {0, 0, 1, 1, 2, 2};
            gun -= 16;
            Sprite(out, x + 210, 261 - gun, base + 2, -32, pal, 0);
            Sprite(out, x + 210, 235 - gun, base + 3 + frames[k % 6], -32, pal, 0);
        }
        else
        {
            Sprite(out, x + 210, 261 - gun, base, shade, pal, 0);
        }
        break;
    case DN_WEAPON_TRIPBOMB:
        x += 8;
        gun -= 10;
        if (k > 6)
        {
            arc += k << 3;
        }
        else if (k < 4)
        {
            Sprite(out, x + 142, arc + 234 - gun, base + 3, shade, pal, 0);
        }
        Sprite(out, x + 130, arc + 249 - gun, base + (k >> 2), shade, pal, 0);
        Sprite(out, x + 152, arc + 249 - gun, base + (k >> 2), shade, pal, 1);
        break;
    case DN_WEAPON_SHRINKER:
    case DN_WEAPON_GROW:
        x += 28;
        arc += 18;
        Sprite(out, x + 184, arc + 240 - gun, base + (k ? 3 + (k & 3) : 2),
               k ? -32 : 16 - (Sine(v->sway) >> 10), v->weapon == DN_WEAPON_GROW ? 2 : 0, 0);
        Sprite(out, x + 188, arc + 240 - gun,
               base + (v->weapon == DN_WEAPON_GROW ? (k ? -1 : -2) : (k ? 1 : 0)), shade, pal, 0);
        break;
    }
}
