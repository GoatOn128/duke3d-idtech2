/* Map placement restrictions from the original GAME.cpp spawn cases. */
#include "dncon.h"

static const char *pickups[] = {
    "HEAVYHBOMB",  "ATOMICHEALTH",   "STEROIDS",       "HEATSENSOR",
    "SHIELD",      "AIRTANK",        "TRIPBOMBSPRITE", "JETPACK",
    "HOLODUKE",    "FIRSTGUNSPRITE", "CHAINGUNSPRITE", "SHOTGUNSPRITE",
    "RPGSPRITE",   "SHRINKERSPRITE", "FREEZESPRITE",   "DEVISTATORSPRITE",
    "SHOTGUNAMMO", "FREEZEAMMO",     "HBOMBAMMO",      "CRYSTALAMMO",
    "GROWAMMO",    "BATTERYAMMO",    "DEVISTATORAMMO", "RPGAMMO",
    "BOOTS",       "AMMO",           "AMMOLOTS",       "COLA",
    "FIRSTAID",    "SIXPAK"};

static int IsPickup(int tile)
{
    for (int i = 0; i < sizeof(pickups) / sizeof(pickups[0]); i++)
    {
        if (tile == DN_ConSymbol(pickups[i], -1))
        {
            return 1;
        }
    }
    return 0;
}

int DN_IsDeathmatch(void)
{
    return !gi.cvar("coop", "0", 0)->value &&
           (dn_maxclients > 1 || gi.cvar("deathmatch", "0", 0)->value);
}

void DN_InitPickup(edict_t *entity, const edict_t *parent)
{
    int card = entity->tile == DN_ConSymbol("ACCESSCARD", 60);
    if (!IsPickup(entity->tile) && !card)
    {
        return;
    }
    if (!card)
    {
        /* A pickup's map palette marks multiplayer placement, not its color. */
        entity->pal = 0;
    }
    if (entity->tile == DN_ConSymbol("HEAVYHBOMB", 26))
    {
        return;
    }
    entity->xrepeat = entity->yrepeat = entity->tile == DN_ConSymbol("AMMO", 40) ? 16 : 32;
    entity->shade = -17;
    entity->cstat = 0;
    if (parent)
    {
        entity->lotag = 0;
        entity->s.origin[2] += 32;
        entity->zspeed = 1024.f / 256;
        /* spawn() advances the initial -1024 zvel by half a movesprite tick. */
        entity->s.origin[2] += entity->zspeed / 2;
        entity->cstat = DN_Random() & 4;
    }
    else
    {
        DN_Fall(entity);
    }
    if (entity->tile == DN_ConSymbol("ATOMICHEALTH", 100))
    {
        entity->cstat |= 128;
    }
}

int DN_AllowMapSprite(const dn_sprite_t *sprite)
{
    int deathmatch = DN_IsDeathmatch();
    int multiplayer = dn_maxclients > 1 || deathmatch;
    if (deathmatch && DN_IsEnemy(sprite->picnum))
    {
        return 0;
    }
    /* GAME.cpp treats palette-marked wall switches as deathmatch-only. Access
       readers use their palette to select a key, so must not be filtered. */
    if ((sprite->cstat & 16) && DN_IsWallSwitch(sprite->picnum) &&
        sprite->picnum != DN_ConSymbol("ACCESSSWITCH", 130) &&
        sprite->picnum != DN_ConSymbol("ACCESSSWITCH2", 170) && sprite->pal && !deathmatch)
    {
        return 0;
    }
    int skill = (int)gi.cvar("skill", "1", 0)->value + 1;
    if (IsPickup(sprite->picnum))
    {
        return (multiplayer || !sprite->pal) && sprite->lotag <= skill;
    }
    if (sprite->picnum == DN_ConSymbol("ACCESSCARD", 60))
    {
        return !deathmatch;
    }
    static const char *multiplayer_props[] = {
        "DUKETAG",     "SIGN1",  "SIGN2",  "REACTOR", "REACTOR2", "RECON",  "CAMERAPOLE",
        "GENERICPOLE", "CRACK1", "CRACK2", "CRACK3",  "CRACK4",   "FIREEXT"};
    for (int i = 0; i < sizeof(multiplayer_props) / sizeof(multiplayer_props[0]); i++)
    {
        if (sprite->picnum == DN_ConSymbol(multiplayer_props[i], -1))
        {
            return multiplayer || !sprite->pal;
        }
    }
    int camera = DN_ConSymbol("CAMERA1", 621);
    if (sprite->picnum >= camera && sprite->picnum <= camera + 4)
    {
        return multiplayer || !sprite->pal;
    }
    return 1;
}
