/* Renderer-independent original weapon sprite composition. */
#ifndef DN_WEAPON_VIEW_H
#define DN_WEAPON_VIEW_H
#include "../qcommon/dnplayerstate.h"

typedef struct
{
    int weapon, base, phase, sway, position, side, access, shade, palette;
} dn_weapon_view_t;

typedef struct
{
    int tile, x, y, shade, palette, mirror;
} dn_weapon_sprite_t;

typedef struct
{
    int count;
    dn_weapon_sprite_t sprites[8];
} dn_weapon_sprites_t;

void DN_BuildWeaponView(const dn_weapon_view_t *view, dn_weapon_sprites_t *result);
#endif
