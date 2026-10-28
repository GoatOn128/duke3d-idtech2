/* GAME.cpp animatesprites lighting classifications; actor shade stays unchanged. */
#include "dncon.h"

enum
{
    KEEP = 1,
    SECTOR = 2,
    FIXED16 = 4,
    SPLAT = 8,
    POOL = 16,
    GIB = 32,
    BRIGHT = 64,
    OWN_PALETTE = 128,
    KEEP127 = 256
};

static uint16_t rules[DN_MAX_TILES];

static void Rule(const char *name, int flags)
{
    int tile = DN_ConSymbol(name, -1);
    if (tile >= 0 && tile < DN_MAX_TILES)
    {
        rules[tile] |= flags;
    }
}

void DN_InitLighting(void)
{
    memset(rules, 0, sizeof(rules));
    const char *keep[] = {"NEON1",
                          "NEON2",
                          "NEON3",
                          "NEON4",
                          "NEON5",
                          "NEON6",
                          "RESPAWNMARKERRED",
                          "RESPAWNMARKERYELLOW",
                          "RESPAWNMARKERGREEN"};
    const char *pools[] = {"BLOODPOOL", "FOOTPRINTS", "FOOTPRINTS2", "FOOTPRINTS3", "FOOTPRINTS4"};
    const char *splats[] = {"BLOODSPLAT1", "BLOODSPLAT2", "BLOODSPLAT3", "BLOODSPLAT4"};
    const char *fixed[] = {"BULLETHOLE", "CRACK1", "CRACK2", "CRACK3", "CRACK4"};
    const char *gibs[] = {"JIBS1",      "JIBS2",      "JIBS3",   "JIBS4",   "JIBS5",
                          "JIBS6",      "HEADJIB1",   "ARMJIB1", "LEGJIB1", "LIZMANHEAD1",
                          "LIZMANARM1", "LIZMANLEG1", "DUKELEG", "DUKEGUN", "DUKETORSO"};
    const char *bright[] = {
        "EXPLOSION2",     "EXPLOSION2BOT", "FREEZEBLAST", "ATOMICHEALTH",      "FIRELASER",
        "SHRINKSPARK",    "GROWSPARK",     "CHAINGUN",    "SHRINKEREXPLOSION", "RPG",
        "FLOORFLAME",     "FIRE",          "FIRE2",       "BURNING",           "BURNING2",
        "COOLEXPLOSION1", "LASERLINE"};
    const char *own_palette[] = {"PUKE",         "MONEY",       "MAIL",       "PAPER",
                                 "TRIPBOMB",     "FORCESPHERE", "BURNING",    "BURNING2",
                                 "ATOMICHEALTH", "CRYSTALAMMO", "VIEWSCREEN", "VIEWSCREEN2"};
    for (int i = 0; i < sizeof(keep) / sizeof(keep[0]); i++)
    {
        Rule(keep[i], KEEP);
    }
    for (int i = 0; i < sizeof(pools) / sizeof(pools[0]); i++)
    {
        Rule(pools[i], SECTOR | POOL | OWN_PALETTE);
    }
    for (int i = 0; i < sizeof(splats) / sizeof(splats[0]); i++)
    {
        Rule(splats[i], FIXED16 | SPLAT);
    }
    for (int i = 0; i < sizeof(fixed) / sizeof(fixed[0]); i++)
    {
        Rule(fixed[i], FIXED16);
    }
    for (int i = 0; i < sizeof(gibs) / sizeof(gibs[0]); i++)
    {
        Rule(gibs[i], GIB);
    }
    for (int i = 0; i < sizeof(bright) / sizeof(bright[0]); i++)
    {
        Rule(bright[i], BRIGHT);
    }
    for (int i = 0; i < sizeof(own_palette) / sizeof(own_palette[0]); i++)
    {
        Rule(own_palette[i], OWN_PALETTE);
    }
    Rule("PUKE", SECTOR | KEEP127);
    int slime = DN_ConSymbol("GREENSLIME", 2370);
    for (int i = 0; i < 8 && slime + i < DN_MAX_TILES; i++)
    {
        rules[slime + i] |= SECTOR;
    }
    Rule("SCRAP1", GIB);
    Rule("SCRAP2", GIB);
    Rule("SCRAP3", GIB);
    Rule("SCRAP4", GIB);
    Rule("SCRAP5", GIB);
    int scrap = DN_ConSymbol("SCRAP6", 2390);
    for (int i = scrap; i <= scrap + 7 && i < DN_MAX_TILES; i++)
    {
        rules[i] |= GIB;
    }
}

int DN_SpriteLighting(const edict_t *entity, int *shade)
{
    int rule = entity->tile >= 0 && entity->tile < DN_MAX_TILES ? rules[entity->tile] : 0;
    int flags = rule & OWN_PALETTE ? RF_DUKE_OWN_PALETTE : 0;
    *shade = entity->shade;
    int inherit = (rule & SECTOR) || (!(rule & KEEP) && !(entity->cstat & 16) &&
                                      !(entity->enemy && entity->health > 0) && !entity->client);
    if ((rule & (POOL | KEEP127)) && entity->shade == 127)
    {
        inherit = 0;
    }
    if (inherit)
    {
        flags |= RF_DUKE_SECTOR_SHADE;
        *shade = 0;
    }
    if (rule & FIXED16)
    {
        flags &= ~RF_DUKE_SECTOR_SHADE;
        *shade = (rule & SPLAT) && entity->pal == 6 ? -127 : 16;
    }
    if (rule & GIB)
    {
        if (entity->pal == 6)
        {
            flags &= ~RF_DUKE_SECTOR_SHADE;
            *shade = -120;
        }
        *shade -= 6;
    }
    if ((rule & BRIGHT) || ((rule & POOL) && entity->pal == 6))
    {
        flags &= ~RF_DUKE_SECTOR_SHADE;
        *shade = -127;
    }
    return flags;
}
