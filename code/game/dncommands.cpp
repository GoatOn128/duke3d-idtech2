/* Dedicated-server game inspection and administration commands. */
#include "dnlocal.h"
#include "dncon.h"
#include "../qcommon/dnplayermove.h"

void DN_ServerCommand(void)
{
    if (!strcmp(gi.argv(1), "player_place") && gi.argc() == 9)
    {
        int index = atoi(gi.argv(2));
        vec3_t point = {(float)atof(gi.argv(3)), (float)atof(gi.argv(4)), (float)atof(gi.argv(5))};
        float yaw = (float)atof(gi.argv(6)), pitch = (float)atof(gi.argv(7));
        /* The last argument is an explicit sector hint for overlapping Build sectors. */
        int sector = atoi(gi.argv(8));
        if (index < 1 || index > dn_maxclients || !dn_edicts[index].inuse || !isfinite(point[0]) ||
            !isfinite(point[1]) || !isfinite(point[2]) || !isfinite(yaw) || !isfinite(pitch) ||
            DN_FindSector(&dn_world, point, sector) < 0)
        {
            return;
        }
        edict_t *player = &dn_edicts[index];
        gclient_t *client = player->client;
        VectorCopy(point, player->s.origin);
        player->sector = sector;
        VectorClear(client->ps.pmove.velocity);
        DN_ResetPlayerMotion(&client->ps.prediction);
        VectorSet(client->ps.viewangles, pitch, yaw, 0);
        for (int axis = 0; axis < 3; axis++)
        {
            client->ps.pmove.origin[axis] = (int32_t)lroundf(point[axis] * 8);
            player->s.origin[axis] = client->ps.pmove.origin[axis] / 8.f;
            client->ps.pmove.delta_angles[axis] =
                ANGLE2SHORT(client->ps.viewangles[axis]) - client->lastcmd.angles[axis];
        }
        DN_Link(player);
        DN_UpdatePlayerStats(player);
        return;
    }
    if (!strcmp(gi.argv(1), "sprite_damage") && gi.argc() == 4)
    {
        edict_t *entity = DN_MapSprite(atoi(gi.argv(2)));
        if (entity)
        {
            DN_Damage(entity, &dn_edicts[0], atoi(gi.argv(3)), DN_ConSymbol("SHOTSPARK1", 2595));
        }
        return;
    }
    if (!strcmp(gi.argv(1), "sprite_info") && gi.argc() == 3)
    {
        int index = atoi(gi.argv(2));
        edict_t *entity = DN_MapSprite(index);
        if (entity)
        {
            gi.dprintf("Duke map sprite %d: tile %d solid %d repeat %d %d shade %d sector %d\n",
                       index, entity->tile, entity->solid, entity->xrepeat, entity->yrepeat,
                       entity->shade, entity->sector);
            gi.dprintf("Duke actor %d: position %.3f %.3f %.3f dormant %d count %d ai %d "
                       "health %d palette %d projectiles %d\n",
                       index, entity->s.origin[0], entity->s.origin[1], entity->s.origin[2],
                       entity->dormant, entity->count, entity->ai, entity->health, entity->pal,
                       entity->projectile);
            float floor, ceiling;
            DN_ActorZRange(entity, &floor, &ceiling);
            gi.dprintf("Duke actor state %d: action %d frame %d clock %d actioncount %d floor %.3f "
                       "ceiling %.3f\n",
                       index, entity->action, entity->s.frame, entity->animclock,
                       entity->actioncount, floor, ceiling);
            gi.dprintf("Duke native state %d: phase %d clock %d turn %d locator %d angle %d "
                       "speed %.3f views %d\n",
                       index, entity->native_state[0], entity->native_state[2],
                       entity->native_state[3], entity->native_state[4], entity->native_angle,
                       entity->speed, entity->viewtype);
        }
        else
        {
            gi.dprintf("Duke map sprite %d: inactive\n", index);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "sprite_spawn") && gi.argc() == 4)
    {
        int index = atoi(gi.argv(2));
        int tile = DN_ConSymbol(gi.argv(3), -1);
        edict_t *parent =
            index < 0 && index >= -dn_maxclients ? &dn_edicts[-index] : DN_MapSprite(index);
        if (parent && parent->inuse && tile >= 0 && tile < DN_MAX_TILES)
        {
            edict_t *entity = DN_SpawnSprite(parent, tile);
            gi.dprintf("Duke spawned tile %d: repeat %d %d palette %d cstat %d height %.3f\n",
                       entity->tile, entity->xrepeat, entity->yrepeat, entity->pal, entity->cstat,
                       entity->s.origin[2]);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "sprite_probe") && gi.argc() == 3)
    {
        int index = atoi(gi.argv(2));
        edict_t *entity = DN_MapSprite(index);
        edict_t *player = DN_NearestPlayer(entity ? entity : dn_edicts, NULL);
        if (entity && player)
        {
            vec3_t start, end;
            VectorCopy(player->s.origin, start);
            start[2] += player->client->ps.viewoffset[2];
            VectorCopy(entity->s.origin, end);
            end[2] += (entity->mins[2] + entity->maxs[2]) / 2;
            trace_t trace = gi.trace(start, vec3_origin, vec3_origin, end, player, MASK_SHOT);
            gi.dprintf("Duke sprite probe %d: hit %d fraction %.4f\n", index, trace.ent == entity,
                       trace.fraction);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "weapon_view") && gi.argc() == 4)
    {
        int weapon = atoi(gi.argv(2)), frame = atoi(gi.argv(3));
        if (weapon < 0 || weapon >= DN_WEAPON_COUNT || frame < 0 || frame > 30)
        {
            return;
        }
        for (int i = 1; i <= dn_maxclients; i++)
        {
            edict_t *player = &dn_edicts[i];
            if (!player->inuse || !player->client)
            {
                continue;
            }
            gclient_t *client = player->client;
            client->weapons = (1 << DN_WEAPON_COUNT) - 1;
            client->weapon = weapon;
            client->weapon_frame = frame;
            client->weapon_sway = 1024;
            client->weapon_position = 0;
            client->ammo[weapon] = 99;
            DN_UpdatePlayerStats(player);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "worldinfo"))
    {
        gi.dprintf("Duke game world %08x\n", DN_WorldChecksum(&dn_world));
        return;
    }
    if (!strcmp(gi.argv(1), "sector_visibility") && gi.argc() == 4)
    {
        int sector = atoi(gi.argv(2)), visibility = atoi(gi.argv(3));
        if (sector >= 0 && sector < dn_world.numsectors && visibility >= 0 && visibility <= 255)
        {
            dn_world.sectors[sector].visibility = visibility;
            DN_PublishSector(sector);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "sector_shade") && gi.argc() == 5)
    {
        int sector = atoi(gi.argv(2)), floor = atoi(gi.argv(3)), ceiling = atoi(gi.argv(4));
        if (sector >= 0 && sector < dn_world.numsectors && floor >= -128 && floor <= 127 &&
            ceiling >= -128 && ceiling <= 127)
        {
            dn_world.sectors[sector].floorshade = floor;
            dn_world.sectors[sector].ceilingshade = ceiling;
            DN_PublishSector(sector);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "sector_set") && gi.argc() == 5)
    {
        int sector = atoi(gi.argv(2));
        if (sector >= 0 && sector < dn_world.numsectors)
        {
            dn_world.sectors[sector].floorz = atoi(gi.argv(3));
            dn_world.sectors[sector].ceilingz = atoi(gi.argv(4));
            DN_PublishSector(sector);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "wall_set") && gi.argc() == 5)
    {
        DN_SetWallPosition(atoi(gi.argv(2)), atoi(gi.argv(3)), atoi(gi.argv(4)));
        return;
    }
    if (!strcmp(gi.argv(1), "wall_style") && gi.argc() == 8)
    {
        int index = atoi(gi.argv(2));
        int tile = atoi(gi.argv(3)), flags = atoi(gi.argv(4));
        int overlay = atoi(gi.argv(5)), shade = atoi(gi.argv(6)), palette = atoi(gi.argv(7));
        if (index >= 0 && index < dn_world.numwalls && tile >= 0 && tile < DN_MAX_TILES &&
            overlay >= 0 && overlay < DN_MAX_TILES && flags >= 0 && flags <= 65535 &&
            shade >= -128 && shade <= 127 && palette >= 0 && palette <= 255)
        {
            dn_wall_t *wall = &dn_world.walls[index];
            wall->picnum = tile;
            wall->cstat = flags;
            wall->overpicnum = overlay;
            wall->shade = shade;
            wall->pal = palette;
            DN_PublishWallStyle(index);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "sector_activate") && gi.argc() == 3)
    {
        int sector = atoi(gi.argv(2));
        if (sector >= 0 && sector < dn_world.numsectors)
        {
            DN_ActivateSector(sector);
        }
        return;
    }
    if (!strcmp(gi.argv(1), "sector_info") && gi.argc() == 3)
    {
        int index = atoi(gi.argv(2));
        if (index >= 0 && index < dn_world.numsectors)
        {
            const dn_sector_t *sector = &dn_world.sectors[index];
            gi.dprintf("Duke sector %d: floor %.3f ceiling %.3f moving %d\n", index,
                       -sector->floorz / 256.f, -sector->ceilingz / 256.f, DN_SectorMoving(index));
            for (int i = 1; i < globals.num_edicts; i++)
            {
                const edict_t *entity = &dn_edicts[i];
                if (entity->inuse && entity->sector == index &&
                    !(entity->s.renderfx & RF_DUKE_WORLD))
                {
                    gi.dprintf(
                        "Duke sector actor %d: tile %d z %.3f solid %d cstat %u bounds %.3f %.3f\n",
                        i, entity->tile, entity->s.origin[2], entity->solid, entity->cstat,
                        entity->mins[2], entity->maxs[2]);
                }
            }
        }
        return;
    }
    if (!strcmp(gi.argv(1), "coninfo"))
    {
        gi.dprintf("Original CON bytecode runs server-side at 30 Hz; map %s, %d entities\n",
                   dn_current_map, globals.num_edicts);
    }
    else if (!strcmp(gi.argv(1), "playerinfo"))
    {
        for (int i = 1; i <= dn_maxclients; i++)
        {
            edict_t *player = &dn_edicts[i];
            if (player->inuse)
            {
                gi.dprintf("Duke player %d: position %.3f %.3f %.3f, sector %d, health %d\n", i,
                           player->s.origin[0], player->s.origin[1], player->s.origin[2],
                           player->sector, player->client->health);
                gi.dprintf("Duke player bathroom %d: cooldown %d weapon_position %d ammo %d\n", i,
                           player->client->last_pissed_time, player->client->weapon_position,
                           player->client->ammo[player->client->weapon]);
            }
        }
    }
}
