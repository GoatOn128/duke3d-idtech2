/* Server weapon sequences follow PLAYER.cpp's kickback_pic clock. */
#include "dnlocal.h"
#include "dncon.h"

static void WeaponSound(edict_t *player, const char *name)
{
    DN_PlaySound(player, DN_ConSymbol(name, -1), 0, 0);
}

void DN_SelectWeapon(edict_t *player, int weapon)
{
    gclient_t *client = player->client;
    if (client->last_pissed_time > DN_TOILET_ACTIVE_END || weapon < 0 ||
        weapon >= DN_WEAPON_COUNT || weapon == client->weapon || !(client->weapons & (1 << weapon)))
    {
        return;
    }
    client->weapon = weapon;
    client->weapon_frame = client->weapon_hold = 0;
    client->weapon_position = 10;
}

static void CheckWeapon(edict_t *player)
{
    static const int order[] = {DN_WEAPON_RPG,     DN_WEAPON_CHAINGUN, DN_WEAPON_DEVASTATOR,
                                DN_WEAPON_SHOTGUN, DN_WEAPON_FREEZE,   DN_WEAPON_SHRINKER,
                                DN_WEAPON_GROW,    DN_WEAPON_PIPEBOMB, DN_WEAPON_PISTOL,
                                DN_WEAPON_KNEE};
    gclient_t *client = player->client;
    if (!client->weapon || client->weapon == DN_WEAPON_REMOTE || client->ammo[client->weapon] > 0)
    {
        return;
    }
    for (int i = 0; i < sizeof(order) / sizeof(order[0]); i++)
    {
        int weapon = order[i];
        if ((client->weapons & (1 << weapon)) && (!weapon || client->ammo[weapon] > 0))
        {
            DN_SelectWeapon(player, weapon);
            return;
        }
    }
}

static int Fire(edict_t *player, const char *tile, const char *sound, int consume)
{
    gclient_t *client = player->client;
    if (consume && client->ammo[client->weapon] <= 0)
    {
        return 0;
    }
    if (consume)
    {
        client->ammo[client->weapon]--;
    }
    DN_Shoot(player, DN_ConSymbol(tile, -1));
    if (sound)
    {
        WeaponSound(player, sound);
    }
    return 1;
}

void DN_WeaponTick(edict_t *player)
{
    gclient_t *client = player->client;
    int attack = (client->buttons & BUTTON_ATTACK) != 0;
    int *frame = &client->weapon_frame;
    if (client->health <= 0)
    {
        *frame = 0;
        return;
    }
    /* Input packets advance native sway at 30 Hz in shared movement. Sampling
       the accumulated displacement three times per server frame causes a jerk. */
    client->weapon_sway = client->ps.prediction.motion_sway & 4095;
    if (client->last_pissed_time > DN_TOILET_ACTIVE_END)
    {
        client->weapon_position = DN_Max(-9, client->weapon_position - 1);
        return;
    }
    if (client->weapon_position > 0)
    {
        client->weapon_position--;
        return;
    }
    if (client->access_ticks)
    {
        return;
    }
    if (!*frame)
    {
        if (!attack)
        {
            return;
        }
        CheckWeapon(player);
        if (client->weapon != DN_WEAPON_KNEE && client->weapon != DN_WEAPON_REMOTE &&
            client->ammo[client->weapon] <= 0)
        {
            return;
        }
        *frame = 1;
        client->weapon_hold = 0;
        switch (client->weapon)
        {
        case DN_WEAPON_PISTOL:
            client->ammo[DN_WEAPON_PISTOL]--;
            break;
        case DN_WEAPON_SHRINKER:
            WeaponSound(player, "SHRINKER_FIRE");
            break;
        case DN_WEAPON_GROW:
            WeaponSound(player, "EXPANDERSHOOT");
            break;
        case DN_WEAPON_FREEZE:
        case DN_WEAPON_DEVASTATOR:
            WeaponSound(player, "CAT_FIRE");
            if (client->weapon == DN_WEAPON_DEVASTATOR)
            {
                client->weapon_side ^= 1;
            }
            break;
        }
        return;
    }
    switch (client->weapon)
    {
    case DN_WEAPON_PISTOL:
        if (*frame == 1)
        {
            Fire(player, "SHOTSPARK1", "PISTOL_FIRE", 0);
        }
        ++*frame;
        if (*frame >= 5)
        {
            if (client->ammo[DN_WEAPON_PISTOL] <= 0 || client->ammo[DN_WEAPON_PISTOL] % 12)
            {
                *frame = 0;
                CheckWeapon(player);
            }
            else if (*frame == 5)
            {
                WeaponSound(player, "EJECT_CLIP");
            }
            else if (*frame == 8)
            {
                WeaponSound(player, "INSERT_CLIP");
            }
        }
        if (*frame == 27)
        {
            *frame = 0;
            CheckWeapon(player);
        }
        break;
    case DN_WEAPON_SHOTGUN:
        ++*frame;
        if (*frame == 4)
        {
            /* DN_Shoot emits the original seven pellets as one discharge. */
            Fire(player, "SHOTGUN", "SHOTGUN_FIRE", 1);
        }
        if (*frame == 15)
        {
            WeaponSound(player, "SHOTGUN_COCK");
        }
        if (*frame == 17 || *frame == 20 || *frame == 24)
        {
            ++*frame;
        }
        if (*frame == 31)
        {
            *frame = 0;
            CheckWeapon(player);
        }
        break;
    case DN_WEAPON_CHAINGUN:
        ++*frame;
        if (*frame <= 12 && *frame % 3 == 0)
        {
            if (!Fire(player, "CHAINGUN", "CHAINGUN_FIRE", 1) || !attack)
            {
                *frame = 0;
                CheckWeapon(player);
            }
        }
        else if (*frame > 12)
        {
            *frame = attack ? 1 : 0;
        }
        break;
    case DN_WEAPON_RPG:
        ++*frame;
        if (*frame == 4)
        {
            Fire(player, "RPG", "RPG_SHOOT", 1);
        }
        if (*frame == 20)
        {
            *frame = 0;
            CheckWeapon(player);
        }
        break;
    case DN_WEAPON_SHRINKER:
    case DN_WEAPON_GROW:
        if (*frame > (client->weapon == DN_WEAPON_GROW ? 3 : 10))
        {
            Fire(player, client->weapon == DN_WEAPON_GROW ? "GROWSPARK" : "SHRINKSPARK", NULL, 1);
            *frame = 0;
            CheckWeapon(player);
        }
        else
        {
            ++*frame;
        }
        break;
    case DN_WEAPON_DEVASTATOR:
        ++*frame;
        if (*frame & 1)
        {
            Fire(player, "RPG", NULL, 1);
        }
        if (*frame > 5)
        {
            *frame = 0;
            CheckWeapon(player);
        }
        break;
    case DN_WEAPON_FREEZE:
        if (*frame < 4)
        {
            if (++*frame == 3)
            {
                Fire(player, "FREEZEBLAST", NULL, 1);
            }
        }
        else
        {
            *frame = attack && client->ammo[DN_WEAPON_FREEZE] > 0 ? 1 : 0;
            if (*frame)
            {
                WeaponSound(player, "CAT_FIRE");
            }
            else
            {
                CheckWeapon(player);
            }
        }
        break;
    case DN_WEAPON_KNEE:
        ++*frame;
        if (*frame == 7)
        {
            Fire(player, "KNEE", NULL, 0);
        }
        if (*frame == 14)
        {
            *frame = attack ? 1 + (DN_Random() & 3) : 0;
        }
        break;
    case DN_WEAPON_PIPEBOMB:
        if (*frame == 6 && attack)
        {
            if (client->weapon_hold < 64)
            {
                client->weapon_hold++;
            }
            break;
        }
        ++*frame;
        if (*frame == 12)
        {
            Fire(player, "HEAVYHBOMB", NULL, 1);
        }
        if (*frame > 19)
        {
            client->weapons |= 1 << DN_WEAPON_REMOTE;
            DN_SelectWeapon(player, DN_WEAPON_REMOTE);
        }
        break;
    case DN_WEAPON_REMOTE:
        ++*frame;
        if (*frame == 2)
        {
            DN_DetonatePipebombs(player);
        }
        if (*frame == 10)
        {
            *frame = 0;
            if (client->ammo[DN_WEAPON_PIPEBOMB] > 0)
            {
                DN_SelectWeapon(player, DN_WEAPON_PIPEBOMB);
            }
            else
            {
                client->weapon = DN_WEAPON_KNEE;
                CheckWeapon(player);
            }
        }
        break;
    case DN_WEAPON_TRIPBOMB:
        if (*frame == 3)
        {
            if (client->ammo[DN_WEAPON_TRIPBOMB] > 0 && DN_PlaceTripbomb(player))
            {
                client->ammo[DN_WEAPON_TRIPBOMB]--;
            }
            else
            {
                *frame = 0;
                break;
            }
        }
        if (++*frame > 16)
        {
            *frame = 0;
            CheckWeapon(player);
        }
        break;
    }
}

void DN_UpdatePlayerStats(edict_t *player)
{
    gclient_t *client = player->client;
    short *stats = client->ps.stats;
    dn_prediction_t *prediction = &client->ps.prediction;
    prediction->version = DN_PREDICTION_VERSION;
    prediction->tick = (uint32_t)lroundf(dn_time * DN_SIMULATION_HZ);
    prediction->seed = DN_RandomState();
    prediction->program = DN_ConChecksum();
    prediction->world = DN_WorldChecksum(&dn_world);
    prediction->sector = player->sector;
    prediction->transport_hold = client->transport_hold;
    prediction->jetpack_on = client->jetpack_on;
    prediction->tile = player->tile;
    prediction->health = player->health;
    prediction->action = player->action;
    prediction->move = player->move;
    prediction->ai = player->ai;
    prediction->moveflags = player->moveflags;
    prediction->count = player->count;
    prediction->actioncount = player->actioncount;
    prediction->animframe = player->animframe;
    prediction->animclock = player->animclock;
    prediction->pal = player->pal;
    prediction->lastpal = player->lastpal;
    prediction->cstat = player->cstat;
    prediction->xrepeat = player->xrepeat;
    prediction->yrepeat = player->yrepeat;
    prediction->hitweapon = player->hitweapon;
    prediction->lastweapon = player->lastweapon;
    prediction->spawnedby = player->spawnedby;
    prediction->stayput = player->stayput;
    prediction->clipdist = player->clipdist;
    prediction->motion_steroids = client->inventory[0];
    stats[STAT_HEALTH] = client->health;
    stats[STAT_AMMO] =
        client->ammo[client->weapon == DN_WEAPON_REMOTE ? DN_WEAPON_PIPEBOMB : client->weapon];
    stats[STAT_ARMOR] = client->armor;
    for (int i = 0; i < DN_WEAPON_COUNT; i++)
    {
        stats[DN_AmmoStat(i)] = client->ammo[i];
    }
    stats[DN_STAT_WEAPON] = client->weapon;
    stats[DN_STAT_WEAPON_FRAME] = client->weapon_frame;
    stats[DN_STAT_WEAPON_SIDE] = client->weapon_side;
    stats[DN_STAT_WEAPON_SWAY] = client->weapon_sway;
    stats[DN_STAT_WEAPON_POSITION] = client->weapon_position;
    stats[DN_STAT_ACCESS] = client->access_ticks;
    stats[DN_STAT_WEAPONS] = client->weapons;
    stats[DN_STAT_KEYS] = client->inventory[6];
    static const int inventory_slots[] = {9, 0, 3, 4, 7, 2, 10};
    stats[DN_STAT_INVENTORY] = stats[DN_STAT_INVENTORY_AMOUNT] = 0;
    for (int i = 0; i < sizeof(inventory_slots) / sizeof(inventory_slots[0]); i++)
    {
        int amount = client->inventory[inventory_slots[i]];
        if (amount > 0)
        {
            stats[DN_STAT_INVENTORY] = i + 1;
            stats[DN_STAT_INVENTORY_AMOUNT] = i == 0   ? amount
                                              : i == 1 ? (amount + 3) >> 2
                                              : i == 2 ? (amount + 15) / 24
                                              : i == 3 ? (amount + 15) >> 4
                                              : i == 4 ? amount / 12
                                              : i == 5 ? (amount + 63) >> 6
                                                       : amount >> 1;
            break;
        }
    }
    stats[STAT_FRAGS] = DN_IsDeathmatch() ? client->frags : client->kills;
    DN_PublishPlayerScore(player);
    stats[DN_STAT_FORMAT] = DN_PLAYER_FORMAT;
    client->ps.gunindex = client->health > 0 ? dn_sprite_model : 0;
    static const char *tiles[DN_WEAPON_COUNT] = {
        "KNEE",     "FIRSTGUN",   "SHOTGUN",          "CHAINGUN", "RPGGUN",     "HANDTHROW",
        "SHRINKER", "DEVISTATOR", "HANDHOLDINGLASER", "FREEZE",   "HANDREMOTE", "SHRINKER"};
    client->ps.gunframe = DN_ConSymbol(tiles[client->weapon], 2524);
    stats[DN_STAT_INTERMISSION] = 0;
    if (DN_IntermissionActive())
    {
        DN_SetIntermissionStats(player);
    }
}
