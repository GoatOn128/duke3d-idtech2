/* Duke HUD/weapon values carried by the existing player-state stat delta. */
#ifndef DN_PLAYER_STATE_H
#define DN_PLAYER_STATE_H

enum
{
    DN_WEAPON_KNEE,
    DN_WEAPON_PISTOL,
    DN_WEAPON_SHOTGUN,
    DN_WEAPON_CHAINGUN,
    DN_WEAPON_RPG,
    DN_WEAPON_PIPEBOMB,
    DN_WEAPON_SHRINKER,
    DN_WEAPON_DEVASTATOR,
    DN_WEAPON_TRIPBOMB,
    DN_WEAPON_FREEZE,
    DN_WEAPON_REMOTE,
    DN_WEAPON_GROW,
    DN_WEAPON_COUNT
};

enum
{
    DN_STAT_WEAPON = 18,
    DN_STAT_WEAPON_FRAME,
    DN_STAT_WEAPON_SIDE,
    DN_STAT_WEAPON_SWAY,
    DN_STAT_WEAPONS,
    DN_STAT_KEYS,
    DN_STAT_INVENTORY,
    DN_STAT_INVENTORY_AMOUNT,
    DN_STAT_INVENTORY_FLAGS,
    DN_STAT_WEAPON_POSITION,
    DN_STAT_ACCESS,
    DN_STAT_LANDING,
    DN_STAT_INTERMISSION,
    DN_STAT_FORMAT
};

#define DN_PLAYER_FORMAT 0x444e
#define DN_CS_INTERMISSION (CS_GENERAL + 1)
#define DN_CS_LEVEL_NAME (CS_GENERAL + 2)
#define DN_CS_DEATHMATCH (CS_GENERAL + 3)
#define DN_CS_PLAYER_SCORES (CS_GENERAL + 4)
#define DN_SCORE_PLAYERS 16
/* During intermission the HUD is replaced by results. */
#define DN_STAT_SECRETS DN_STAT_KEYS

static inline int DN_AmmoStat(int weapon)
{
    static const int slots[DN_WEAPON_COUNT] = {0, 2, 4, 6, 7, 8, 9, 10, 11, 12, 16, 17};
    return slots[weapon];
}
#endif
