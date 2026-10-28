/* Client lifecycle, movement, inventory and player commands. */
#include "dnlocal.h"
#include "dncon.h"
#include "../qcommon/dnplayermove.h"

static edict_t *moving_player;

static int IsDeathmatchStart(const dn_sprite_t *sprite)
{
    /* APLAYER lotag 1 is a co-op start. Marker repeat sizes may be zero. */
    return sprite->picnum == DN_ConSymbol("APLAYER", 1405) && sprite->lotag == 0 &&
           sprite->sectnum >= 0 && sprite->sectnum < dn_world.numsectors;
}

static const dn_sprite_t *DeathmatchStart(void)
{
    if (!DN_IsDeathmatch())
    {
        return NULL;
    }
    /* PREMAP.cpp includes the map header start alongside the deathmatch markers. */
    int count = 1;
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        count += IsDeathmatchStart(&dn_world.sprites[i]);
    }
    int spot = DN_Random() % count;
    for (int i = 0; spot && i < dn_world.numsprites; i++)
    {
        if (IsDeathmatchStart(&dn_world.sprites[i]) && --spot == 0)
        {
            return &dn_world.sprites[i];
        }
    }
    return NULL;
}

static trace_t PlayerTrace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
    trace_t trace = gi.trace(start, mins, maxs, end, moving_player, MASK_PLAYERSOLID);
    if (trace.ent == dn_edicts)
    {
        trace.ent = (edict_t *)1; /* Shared player mover's native-world sentinel. */
    }
    return trace;
}

void DN_ResetPlayer(edict_t *ent)
{
    gclient_t *c = ent->client;
    if (!c)
    {
        return;
    }
    char name[sizeof(c->name)];
    int connected = c->connected;
    int frags = c->frags;
    int command_yaw = c->lastcmd.angles[YAW];
    memcpy(name, c->name, sizeof(name));
    memset(c, 0, sizeof(*c));
    memcpy(c->name, name, sizeof(name));
    c->connected = connected;
    c->frags = frags;
    DN_ResetPlayerMotion(&c->ps.prediction);
    c->health = DN_ConSetting("MAXPLAYERHEALTH", 100);
    c->armor = DN_ConSetting("STARTARMORHEALTH", 0);
    memset(c->inventory, 0, sizeof(c->inventory));
    c->access_ticks = 0;
    c->access_wall = c->access_sprite = -1;
    c->weapons = (1 << 1) | 1;
    c->weapon = 1;
    c->weapon_frame = c->weapon_side = c->weapon_hold = c->weapon_sway = 0;
    c->weapon_position = 0;
    c->transport_hold = c->jetpack_on = 0;
    memset(c->ammo, 0, sizeof(c->ammo));
    c->ammo[1] = 48;
    ent->inuse = true;
    ent->health = c->health;
    ent->tile = ent->originaltile = DN_ConSymbol("APLAYER", 1405);
    DN_ConInitActor(ent);
    memset(ent->native_state, 0, sizeof(ent->native_state));
    ent->health = c->health;
    ent->xrepeat = 42;
    ent->yrepeat = 36;
    ent->cstat = 257;
    ent->pal = DN_PlayerPalette(ent);
    ent->solid = SOLID_BBOX;
    ent->sector = dn_world.startsector;
    VectorCopy(dn_world.start, ent->s.origin);
    float angle = dn_world.angle;
    const dn_sprite_t *start = DeathmatchStart();
    if (start)
    {
        ent->sector = start->sectnum;
        VectorSet(ent->s.origin, start->x / 16.f, -start->y / 16.f, -start->z / 256.f);
        angle = -start->ang * 360.f / 2048;
    }
    {
        float floor = DN_PlaneZ(&dn_world, ent->sector, 0, ent->s.origin[0], ent->s.origin[1]);
        float ceiling = DN_PlaneZ(&dn_world, ent->sector, 1, ent->s.origin[0], ent->s.origin[1]);
        ent->s.origin[2] = fmaxf(floor + 28, fminf(ceiling - 16, ent->s.origin[2] - 12));
    }
    for (int i = 0; i < 3; i++)
    {
        c->ps.pmove.origin[i] = (int)(ent->s.origin[i] * 8);
        ent->s.origin[i] = c->ps.pmove.origin[i] / 8.f;
    }
    VectorCopy(ent->s.origin, ent->s.old_origin);
    ent->s.angles[YAW] = c->ps.viewangles[YAW] = angle;
    c->ps.pmove.delta_angles[YAW] = ANGLE2SHORT(angle) - command_yaw;
    c->ps.fov = 90;
    c->ps.viewoffset[2] = 12;
    VectorSet(ent->mins, -164.f / 16, -164.f / 16, -28);
    VectorSet(ent->maxs, 164.f / 16, 164.f / 16, 16);
    ent->s.number = (int)(ent - dn_edicts);
    DN_Link(ent);
    DN_UpdatePlayerStats(ent);
}

void DN_RespawnPlayer(edict_t *player)
{
    if (dn_maxclients == 1 && !gi.cvar("deathmatch", "0", 0)->value &&
        !gi.cvar("coop", "0", 0)->value)
    {
        DN_ChangeLevel(0);
    }
    else
    {
        DN_ResetPlayer(player);
    }
}

qboolean DN_ClientConnect(edict_t *ent, char *userinfo)
{
    ent->client = &dn_clients[ent - dn_edicts - 1];
    ent->client->connected = 1;
    snprintf(ent->client->name, sizeof(ent->client->name), "%s",
             Info_ValueForKey(userinfo, "name"));
    return true;
}

void DN_ClientBegin(edict_t *ent)
{
    if (ent->client->restore_player && ent->inuse)
    {
        ent->client->restore_player = false;
        for (int axis = 0; axis < 3; axis++)
        {
            ent->client->ps.pmove.delta_angles[axis] =
                ANGLE2SHORT(ent->client->ps.viewangles[axis]);
        }
        DN_Link(ent);
    }
    else
    {
        DN_ResetPlayer(ent);
    }
    gi.bprintf(PRINT_HIGH, "%s entered Duke3D\n", ent->client->name);
}

void DN_ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
    snprintf(ent->client->name, sizeof(ent->client->name), "%s",
             Info_ValueForKey(userinfo, "name"));
    DN_PublishPlayerScore(ent);
}

void DN_ClientDisconnect(edict_t *ent)
{
    gi.unlinkentity(ent);
    ent->inuse = false;
    ent->client->connected = 0;
    ent->s.modelindex = 0;
    ent->client->frags = 0;
    DN_PublishPlayerScore(ent);
}

void DN_ClientThink(edict_t *ent, usercmd_t *cmd)
{
    pmove_t pm;
    vec3_t previous;
    VectorCopy(ent->s.origin, previous);
    gclient_t *c = ent->client;
    c->lastcmd = *cmd;
    c->oldbuttons = c->buttons;
    c->buttons = cmd->buttons;
    if (DN_IntermissionActive())
    {
        DN_SetIntermissionStats(ent);
        return;
    }
    memset(&pm, 0, sizeof(pm));
    pm.s = c->ps.pmove;
    pm.s.pm_type = c->noclip ? PM_SPECTATOR : (c->health <= 0 ? PM_DEAD : PM_NORMAL);
    pm.s.gravity = gi.cvar("sv_gravity", "800", 0)->value;
    for (int i = 0; i < 3; i++)
    {
        pm.s.origin[i] = (int)(ent->s.origin[i] * 8);
    }
    pm.cmd = *cmd;
    pm.trace = PlayerTrace;
    pm.pointcontents = gi.pointcontents;
    moving_player = ent;
    if (pm.s.pm_type == PM_SPECTATOR)
    {
        DN_ResetPlayerMotion(&c->ps.prediction);
        gi.Pmove(&pm);
    }
    else
    {
        c->ps.prediction.sector = ent->sector;
        c->ps.prediction.jetpack_on = c->jetpack_on;
        c->ps.prediction.motion_steroids = c->inventory[0];
        DN_PlayerMove(&pm, &dn_world, &c->ps.prediction);
        int fall = c->ps.prediction.motion_landing;
        if (fall > 9)
        {
            DN_Damage(ent, &dn_edicts[0], fall > 62 ? 10000 : fall - (DN_Random() & 3), 0);
        }
    }
    c->ps.pmove = pm.s;
    for (int i = 0; i < 3; i++)
    {
        ent->s.origin[i] = pm.s.origin[i] / 8.f;
    }
    VectorCopy(pm.viewangles, c->ps.viewangles);
    VectorCopy(pm.mins, ent->mins);
    VectorCopy(pm.maxs, ent->maxs);
    c->ps.viewoffset[2] = pm.viewheight;
    ent->s.angles[1] = pm.viewangles[1];
    DN_CheckPlayerTransport(ent, previous);
    DN_CheckLevelSector(ent);
    if (DN_IntermissionActive())
    {
        return;
    }
    if (c->health <= 0 && (cmd->buttons & BUTTON_ATTACK) && !(c->oldbuttons & BUTTON_ATTACK))
    {
        DN_RespawnPlayer(ent);
    }
    if ((cmd->buttons & BUTTON_USE) && !(c->oldbuttons & BUTTON_USE))
    {
        DN_Operate(ent);
    }
    DN_UpdatePlayerStats(ent);
    c->ps.blend[3] *= .85f;
    DN_Link(ent);
}

void DN_ClientCommand(edict_t *ent)
{
    char *cmd = gi.argv(0);
    gclient_t *c = ent->client;
    if (DN_IntermissionActive())
    {
        if (!strcmp(cmd, "dn_continue"))
        {
            DN_ContinueIntermission();
        }
        return;
    }
    if (!strcmp(cmd, "restart"))
    {
        DN_RespawnPlayer(ent);
    }
    else if (!strcmp(cmd, "noclip"))
    {
        c->noclip ^= 1;
        gi.cprintf(ent, PRINT_HIGH, "noclip %s\n", c->noclip ? "ON" : "OFF");
    }
    else if (!strcmp(cmd, "god"))
    {
        c->god ^= 1;
        gi.cprintf(ent, PRINT_HIGH, "god %s\n", c->god ? "ON" : "OFF");
    }
    else if (!strcmp(cmd, "use"))
    {
        int weapon = atoi(gi.argv(1));
        if (weapon >= 0 && weapon < DN_MAX_WEAPONS && (c->weapons & (1 << weapon)))
        {
            DN_SelectWeapon(ent, weapon);
        }
    }
    else if (!strcmp(cmd, "weapnext") || !strcmp(cmd, "weapprev"))
    {
        int step = !strcmp(cmd, "weapnext") ? 1 : -1;
        for (int i = 1; i <= DN_MAX_WEAPONS; i++)
        {
            int w = (c->weapon + i * step + DN_MAX_WEAPONS) % DN_MAX_WEAPONS;
            if (c->weapons & (1 << w))
            {
                DN_SelectWeapon(ent, w);
                break;
            }
        }
    }
    else if (!strcmp(cmd, "give"))
    {
        c->health = 200;
        c->armor = 200;
        c->weapons = (1 << DN_MAX_WEAPONS) - 1;
        for (int i = 0; i < DN_MAX_WEAPONS; i++)
        {
            c->ammo[i] = 200;
        }
    }
    else if (!strcmp(cmd, "say") || !strcmp(cmd, "say_team"))
    {
        gi.bprintf(PRINT_CHAT, "%s: %s\n", c->name, gi.args());
    }
}
