/* Tagged switches and use queries adapted from Duke's SECTOR.cpp/PLAYER.cpp.
   The server owns activation, access timing and every resulting world edit. */
#include "dnlocal.h"
#include "dncon.h"

typedef enum
{
    SWITCH_NONE,
    SWITCH_TOGGLE,
    SWITCH_DIP,
    SWITCH_MULTI,
    SWITCH_ACCESS,
    SWITCH_DOOR
} switch_kind_t;

typedef struct
{
    switch_kind_t kind;
    int base, frame;
} switch_info_t;

static switch_info_t SwitchInfo(int tile)
{
    static const char *names[] = {
        "LIGHTSWITCH", "SPACEDOORSWITCH", "SPACELIGHTSWITCH", "FRANKENSTINESWITCH", "DIPSWITCH2",
        "DIPSWITCH3",  "LIGHTSWITCH2",    "POWERSWITCH1",     "LOCKSWITCH1",        "POWERSWITCH2",
        "HANDSWITCH",  "PULLSWITCH",      "SLOTDOOR",         "HANDPRINTSWITCH"};
    static const char *dips[] = {"DIPSWITCH", "TECHSWITCH", "ALIENSWITCH"};
    int base;
    for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    {
        base = DN_ConSymbol(names[i], -2);
        if (tile == base || tile == base + 1)
        {
            return (switch_info_t){SWITCH_TOGGLE, base, tile - base};
        }
    }
    for (int i = 0; i < sizeof(dips) / sizeof(dips[0]); i++)
    {
        base = DN_ConSymbol(dips[i], -2);
        if (tile == base || tile == base + 1)
        {
            return (switch_info_t){SWITCH_DIP, base, tile - base};
        }
    }
    base = DN_ConSymbol("MULTISWITCH", 146);
    if (tile >= base && tile <= base + 3)
    {
        return (switch_info_t){SWITCH_MULTI, base, tile - base};
    }
    if (tile == DN_ConSymbol("ACCESSSWITCH", 130) || tile == DN_ConSymbol("ACCESSSWITCH2", 170))
    {
        return (switch_info_t){SWITCH_ACCESS, tile, 0};
    }
    for (int i = 1; i <= 23; i++)
    {
        char name[24];
        if (i == 13)
        {
            continue;
        }
        snprintf(name, sizeof(name), "DOORTILE%d", i);
        if (tile == DN_ConSymbol(name, -1))
        {
            return (switch_info_t){SWITCH_DOOR, tile, 0};
        }
    }
    return (switch_info_t){SWITCH_NONE, tile, 0};
}

int DN_AccessBit(int palette)
{
    return palette == 0 ? 1 : palette == 21 ? 2 : palette == 23 ? 4 : 0;
}

int DN_IsWallSwitch(int tile)
{
    switch_kind_t kind = SwitchInfo(tile).kind;
    return kind != SWITCH_NONE && kind != SWITCH_DOOR;
}

void DN_InitMapSwitch(edict_t *entity)
{
    switch_info_t info = SwitchInfo(entity->tile);
    if (!(entity->cstat & 16) || info.kind == SWITCH_NONE || info.kind == SWITCH_DOOR)
    {
        return;
    }
    entity->cstat |= 257;
    if (info.kind != SWITCH_ACCESS)
    {
        entity->pal = 0;
    }
}

static int IsActivator(const dn_sprite_t *sprite)
{
    return sprite->picnum == DN_ConSymbol("ACTIVATOR", 2) ||
           sprite->picnum == DN_ConSymbol("ACTIVATORLOCKED", 4);
}

static int TagMoving(int tag)
{
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        const dn_sprite_t *sprite = &dn_world.sprites[i];
        if (IsActivator(sprite) && sprite->lotag == tag && DN_SectorMoving(sprite->sectnum))
        {
            return 1;
        }
    }
    return 0;
}

static int IsForcefield(int tile)
{
    int base = DN_ConSymbol("W_FORCEFIELD", 663);
    return (tile >= base && tile <= base + 2) || tile == DN_ConSymbol("BIGFORCE", 230);
}

void DN_ActivateTag(int tag, edict_t *player)
{
    DN_ActivateLights(tag, 0);
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        dn_sprite_t *sprite = &dn_world.sprites[i];
        dn_sector_t *sector = &dn_world.sectors[sprite->sectnum];
        if (sprite->lotag != tag)
        {
            continue;
        }
        if (sprite->picnum == DN_ConSymbol("ACTIVATORLOCKED", 4))
        {
            sector->lotag ^= 16384;
            if (player && player->client)
            {
                gi.centerprintf(player, "%s", dn_quotes[(sector->lotag & 16384) ? 4 : 8]);
            }
        }
        else if (sprite->picnum == DN_ConSymbol("ACTIVATOR", 2) &&
                 !DN_SectorMoving(sprite->sectnum))
        {
            if ((sprite->hitag == 1 && sector->floorz != sector->ceilingz) ||
                (sprite->hitag == 2 && sector->floorz == sector->ceilingz))
            {
                continue;
            }
            DN_ActivateSector(sprite->sectnum);
            DN_TriggerCharges(sprite->sectnum);
        }
        else if (sprite->picnum == DN_ConSymbol("MASTERSWITCH", 8) && sprite->yvel == 0)
        {
            sprite->yvel = 1;
        }
    }
    for (int i = 0; i < dn_world.numwalls; i++)
    {
        dn_wall_t *wall = &dn_world.walls[i];
        if (wall->lotag == tag && IsForcefield(wall->overpicnum))
        {
            wall->cstat = wall->cstat ? 0 : 85;
            DN_PublishWallStyle(i);
        }
    }
}

static int NextSwitchFrame(int tile, int clicked, int *dips, int *correct, int target)
{
    switch_info_t info = SwitchInfo(tile);
    if (info.kind == SWITCH_DIP)
    {
        (*dips)++;
        if (clicked || info.frame == target)
        {
            (*correct)++;
        }
        return clicked ? info.base + !info.frame : tile;
    }
    if (info.kind == SWITCH_MULTI)
    {
        return info.base + ((info.frame + 1) & 3);
    }
    if (info.kind == SWITCH_TOGGLE || info.kind == SWITCH_ACCESS)
    {
        return info.base + !info.frame;
    }
    return tile;
}

int DN_UseSwitch(edict_t *player, int index, edict_t *sprite)
{
    dn_wall_t *wall = NULL;
    int tile, tag, hitag, palette, dips = 0, correct = 0;
    switch_info_t info;
    if (sprite)
    {
        if (!sprite->inuse)
        {
            return 0;
        }
        tile = sprite->tile;
        tag = sprite->lotag;
        hitag = sprite->hitag;
        palette = sprite->pal;
    }
    else
    {
        if (index < 0 || index >= dn_world.numwalls)
        {
            return 0;
        }
        wall = &dn_world.walls[index];
        tile = wall->picnum;
        tag = wall->lotag;
        hitag = wall->hitag;
        palette = wall->pal;
    }
    info = SwitchInfo(tile);
    if (!tag || info.kind == SWITCH_NONE)
    {
        return 0;
    }
    if (info.kind == SWITCH_ACCESS)
    {
        gclient_t *client = player ? player->client : NULL;
        if (!client)
        {
            return 1;
        }
        if (!client->access_ticks)
        {
            if (client->inventory[6] & DN_AccessBit(palette))
            {
                client->access_ticks = 1;
                client->access_wall = wall ? index : -1;
                client->access_sprite = sprite ? (int)(sprite - dn_edicts) : -1;
            }
            else
            {
                int quote = palette == 21 ? 71 : palette == 23 ? 72 : 70;
                gi.centerprintf(player, "%s", dn_quotes[quote]);
            }
            return 1;
        }
        if (client->access_ticks != 12)
        {
            return 1;
        }
    }
    if (info.kind != SWITCH_DIP && TagMoving(tag))
    {
        return 1;
    }
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        edict_t *entity = &dn_edicts[i];
        if (entity->inuse && !(entity->s.renderfx & RF_DUKE_WORLD) && entity->lotag == tag)
        {
            int next =
                NextSwitchFrame(entity->tile, entity == sprite, &dips, &correct, entity->hitag);
            if (next != entity->tile)
            {
                entity->tile = next;
                DN_Link(entity);
            }
        }
    }
    for (int i = 0; i < dn_world.numwalls; i++)
    {
        dn_wall_t *other = &dn_world.walls[i];
        if (other->lotag == tag)
        {
            int next = NextSwitchFrame(other->picnum, other == wall, &dips, &correct, other->hitag);
            if (next != other->picnum)
            {
                other->picnum = next;
                DN_PublishWallStyle(i);
            }
        }
    }
    if (tag == -1)
    {
        DN_ChangeLevel(1);
        return 1;
    }
    if (info.kind == SWITCH_DIP)
    {
        DN_PlaySound(sprite ? sprite : player,
                     DN_ConSymbol(info.base == DN_ConSymbol("ALIENSWITCH", 1142) ? "ALIEN_SWITCH1"
                                                                                 : "SWITCH_ON",
                                  76),
                     0, 0);
        if (dips != correct)
        {
            return 1;
        }
        DN_PlaySound(player, DN_ConSymbol("END_OF_LEVEL_WARN", 183), 0, 0);
    }
    DN_ActivateTag(tag + (info.kind == SWITCH_MULTI ? info.frame : 0), player);
    if (info.kind != SWITCH_DIP && info.kind != SWITCH_DOOR)
    {
        DN_PlaySound(sprite ? sprite : player, hitag ? hitag : DN_ConSymbol("SWITCH_ON", 76), 0, 0);
    }
    return 1;
}

static int Operator(int sector, int underneath)
{
    int tag = dn_world.sectors[sector].lotag;
    if (tag & 16384)
    {
        return 0;
    }
    switch (tag & 255)
    {
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 22:
    case 26:
        return 1;
    case 9:
    case 20:
    case 21:
    case 23:
    case 25:
    case 29:
        return !underneath;
    }
    return 0;
}

static int HasActivator(int sector, int actor)
{
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        const dn_sprite_t *sprite = &dn_world.sprites[i];
        /* Locked activators only gate Operator's lock bit; after a card unlocks
           them, the door accepts direct use (SECTOR.cpp checksectors).
           Spent masters are retained for saves instead of being deleted. */
        if (sprite->sectnum == sector &&
            (sprite->picnum == DN_ConSymbol("ACTIVATOR", 2) ||
             (!actor && sprite->picnum == DN_ConSymbol("MASTERSWITCH", 8) && sprite->yvel != 2)))
        {
            return 1;
        }
    }
    return 0;
}

static int UseBathroom(edict_t *player, edict_t *fixture)
{
    gclient_t *client = player->client;
    if (!DN_IsBathroomFixture(fixture->tile) || !client || client->health <= 0 ||
        player->xrepeat <= 32)
    {
        return 0;
    }
    if (client->last_pissed_time)
    {
        DN_PlaySound(player, DN_ConSymbol("FLUSH_TOILET", 79), 1, 0);
        return 1;
    }
    DN_PlaySound(player, DN_ConSymbol("DUKE_URINATE", 28), 0, 0);
    client->last_pissed_time = DN_TOILET_COOLDOWN;
    client->transport_hold = 58;
    client->weapon_position = -1;
    client->weapon_frame = client->weapon_hold = 0;
    int maximum = DN_ConSetting("MAXPLAYERHEALTH", 100);
    if (client->health < maximum)
    {
        client->health = DN_Min(maximum, client->health + maximum / 10);
        player->health = client->health;
    }
    DN_UpdatePlayerStats(player);
    return 1;
}

/* Trace a horizontal use ray through wall links. Facing and reach replace the
   old nearest-sector-center heuristic; unrelated rooms cannot receive use. */
static int UseRay(edict_t *player, float height, int actor)
{
    vec3_t eye;
    float yaw = player->s.angles[1] * (float)M_PI / 180;
    float vx = cosf(yaw), vy = sinf(yaw), entered = -.01f;
    int sector = player->sector;
    int exit_only = 0;
    byte visited[DN_MAX_SECTORS] = {};
    VectorCopy(player->s.origin, eye);
    eye[2] += height;
    while (sector >= 0 && !visited[sector])
    {
        const dn_sector_t *s = &dn_world.sectors[sector];
        float nearest = actor ? 48.f : 80.f;
        int hit = -1;
        edict_t *sprite = NULL;
        visited[sector] = 1;
        for (int i = s->wallptr; i < s->wallptr + s->wallnum; i++)
        {
            const dn_wall_t *a = &dn_world.walls[i], *b = &dn_world.walls[a->point2];
            /* Shared door/room boundaries can occupy the same plane. Skip
               portals back into visited rooms, retaining a usable wall switch. */
            if (a->nextsector >= 0 && visited[a->nextsector] &&
                (actor || !a->lotag || SwitchInfo(a->picnum).kind == SWITCH_NONE))
            {
                continue;
            }
            float x = a->x / 16.f - eye[0], y = -a->y / 16.f - eye[1];
            float dx = (b->x - (double)a->x) / 16.f, dy = -(b->y - (double)a->y) / 16.f;
            float cross = vx * dy - vy * dx;
            if (cross >= -.00001f)
            {
                continue;
            }
            float distance = (x * dy - y * dx) / cross;
            float along = (x * vy - y * vx) / cross;
            if (distance + .0001f >= entered && distance <= nearest && along >= 0 && along <= 1)
            {
                nearest = distance;
                hit = i;
            }
        }
        for (int i = dn_maxclients + 1; !actor && i < globals.num_edicts; i++)
        {
            edict_t *entity = &dn_edicts[i];
            float x, y, distance, across, width, size;
            int exit_button = entity->tile == DN_ConSymbol("NUKEBUTTON", 142);
            if (!entity->inuse || (exit_only && !exit_button) || entity->sector != sector ||
                (!entity->lotag && !exit_button) || (entity->s.renderfx & RF_DUKE_WORLD) ||
                (entity->cstat & 32768) ||
                (SwitchInfo(entity->tile).kind == SWITCH_NONE &&
                 !DN_IsBathroomFixture(entity->tile) && !exit_button))
            {
                continue;
            }
            x = entity->s.origin[0] - eye[0];
            y = entity->s.origin[1] - eye[1];
            distance = x * vx + y * vy;
            across = x * vy - y * vx;
            DN_SpriteSize(entity, &width, &size);
            /* Authored wall switches can be exactly coplanar with their wall. */
            if (distance > 0 && distance + .0001f >= entered && distance <= nearest + .001f &&
                fabsf(across) <= width * .353554f &&
                eye[2] >= entity->s.origin[2] + entity->mins[2] &&
                eye[2] <= entity->s.origin[2] + entity->maxs[2])
            {
                nearest = distance;
                sprite = entity;
            }
        }
        if (sprite && sprite->tile == DN_ConSymbol("NUKEBUTTON", 142) && player->client &&
            player->client->health > 0)
        {
            sprite->tile++;
            DN_Link(sprite);
            DN_BeginIntermission(sprite->pal ? sprite->lotag : 0);
            return 1;
        }
        if (exit_only)
        {
            return 0;
        }
        if (sprite && (UseBathroom(player, sprite) || DN_UseSwitch(player, -1, sprite)))
        {
            return 1;
        }
        if (hit < 0)
        {
            break;
        }
        const dn_wall_t *wall = &dn_world.walls[hit];
        if (!actor && wall->lotag && DN_UseSwitch(player, hit, NULL))
        {
            return 1;
        }
        if (wall->nextsector < 0)
        {
            break;
        }
        sector = wall->nextsector;
        const dn_sector_t *target = &dn_world.sectors[sector];
        int actor_target =
            !actor || (!(target->lotag & 32768) &&
                       ((target->lotag & 255) == 23 || target->floorz == target->ceilingz));
        if (actor_target && Operator(sector, 0) && !HasActivator(sector, actor))
        {
            DN_ActivateSector(sector);
            return 1;
        }
        if (actor && target->lotag)
        {
            return 0; /* Native neartag stops at the nearest tagged sector. */
        }
        if (eye[2] <
                DN_PlaneZ(&dn_world, sector, 0, eye[0] + nearest * vx, eye[1] + nearest * vy) ||
            eye[2] > DN_PlaneZ(&dn_world, sector, 1, eye[0] + nearest * vx, eye[1] + nearest * vy))
        {
            break;
        }
        if (wall->cstat & (1 | 16 | 32))
        {
            if (actor || wall->overpicnum)
            {
                break;
            }
            /* Native neartag can reach a recessed nuke panel through its plain
               blocking portal. Other switches still stop at that boundary. */
            exit_only = 1;
        }
        /* Build neartag revisits coincident portal planes in adjacent sectors.
           Advancing past this plane skips a door directly behind the portal. */
        entered = nearest;
    }
    return 0;
}

void DN_Operate(edict_t *player)
{
    if (player->sector < 0 || (player->client && player->client->access_ticks))
    {
        return;
    }
    float eye = player->client ? player->client->ps.viewoffset[2] : 32;
    for (int height = 0; height <= 16; height += 8)
    {
        if (UseRay(player, eye - height, 0))
        {
            return;
        }
    }
    if (Operator(player->sector, 1) && !HasActivator(player->sector, 0))
    {
        DN_ActivateSector(player->sector);
    }
}

void DN_OperateActor(edict_t *entity)
{
    /* GAMEDEF.cpp opcode 66 only searches from an untagged room. Actors cannot
       use switches, call the platform they occupy, or reopen an open door. */
    if (entity->sector >= 0 && dn_world.sectors[entity->sector].lotag == 0)
    {
        UseRay(entity, 32, 1);
    }
}

void DN_InitInteractions(void)
{
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        const edict_t *entity = &dn_edicts[i];
        switch_info_t info = SwitchInfo(entity->tile);
        if (entity->inuse && !(entity->s.renderfx & RF_DUKE_WORLD) && info.kind == SWITCH_TOGGLE &&
            info.frame == 1)
        {
            DN_ActivateLights(entity->lotag, 1);
        }
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        dn_sprite_t *sprite = &dn_world.sprites[i];
        if (sprite->picnum == DN_ConSymbol("MASTERSWITCH", 8))
        {
            sprite->yvel = 0;
        }
        if (sprite->picnum == DN_ConSymbol("ACTIVATORLOCKED", 4))
        {
            dn_world.sectors[sprite->sectnum].lotag |= 16384;
        }
    }
    for (int i = 0; i < dn_world.numwalls; i++)
    {
        dn_wall_t *wall = &dn_world.walls[i];
        int tile = DN_ConSymbol("W_FORCEFIELD", 663);
        if (wall->overpicnum < tile || wall->overpicnum > tile + 2)
        {
            continue;
        }
        wall->cstat = wall->shade > 31 ? 0 : wall->cstat | 341;
        if (wall->lotag && wall->nextwall >= 0)
        {
            dn_world.walls[wall->nextwall].lotag = wall->lotag;
        }
        DN_PublishWallStyle(i);
    }
}

void DN_TickInteractions(void)
{
    DN_TickTouchplates();
    for (int i = 1; i <= dn_maxclients; i++)
    {
        edict_t *player = &dn_edicts[i];
        gclient_t *client = player->client;
        if (!player->inuse || !client)
        {
            continue;
        }
        if (client->last_pissed_time > 0)
        {
            client->last_pissed_time--;
            if (client->last_pissed_time == 26 * 219)
            {
                DN_PlaySound(player, DN_ConSymbol("FLUSH_TOILET", 79), 0, 0);
                DN_PlaySound(player, DN_ConSymbol("DUKE_PISSRELIEF", 218), 0, 0);
            }
            if (client->last_pissed_time == DN_TOILET_ACTIVE_END)
            {
                client->weapon_position = 10;
            }
        }
        if (!client->access_ticks)
        {
            continue;
        }
        if (++client->access_ticks == 12)
        {
            int palette = -1;
            if (client->access_sprite > dn_maxclients && client->access_sprite < globals.num_edicts)
            {
                edict_t *sprite = &dn_edicts[client->access_sprite];
                if (sprite->inuse && SwitchInfo(sprite->tile).kind == SWITCH_ACCESS)
                {
                    palette = sprite->pal;
                    DN_UseSwitch(player, -1, sprite);
                }
            }
            else if (client->access_wall >= 0 && client->access_wall < dn_world.numwalls)
            {
                palette = dn_world.walls[client->access_wall].pal;
                DN_UseSwitch(player, client->access_wall, NULL);
            }
            client->inventory[6] &= ~DN_AccessBit(palette);
            client->access_wall = client->access_sprite = -1;
        }
        if (client->access_ticks > 20)
        {
            client->access_ticks = 0;
        }
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        dn_sprite_t *sprite = &dn_world.sprites[i];
        if (sprite->picnum == DN_ConSymbol("MASTERSWITCH", 8) && sprite->yvel == 1 &&
            --sprite->hitag <= 0)
        {
            DN_ActivateSector(sprite->sectnum);
            DN_TriggerCharges(sprite->sectnum);
            sprite->yvel = 2; /* Fired masters never retrigger. Saved with the raw map sprites. */
        }
    }
}
