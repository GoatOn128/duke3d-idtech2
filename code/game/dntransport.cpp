/* Native SE7/SE23 dry player transports from GAME.cpp/ACTORS.cpp.
   Controllers remain server entities; clients receive the resulting player state. */
#include "dncon.h"
#include "../qcommon/dntransport.h"

static int IsTransport(const edict_t *entity)
{
    return entity->inuse && entity->tile == 1 && (entity->lotag == 7 || entity->lotag == 23);
}

void DN_InitTransports(void)
{
    edict_t **controllers = (edict_t **)(calloc(dn_world.numsprites, sizeof(*controllers)));
    if (dn_world.numsprites && !controllers)
    {
        gi.error("Cannot allocate Duke transporter links");
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        const dn_sprite_t *sprite = &dn_world.sprites[i];
        if (sprite->picnum != 1 || (sprite->lotag != 7 && sprite->lotag != 23))
        {
            continue;
        }
        edict_t *entity = controllers[i] = DN_NewEntity();
        entity->tile = entity->originaltile = 1;
        entity->lotag = sprite->lotag;
        entity->hitag = sprite->hitag;
        entity->sector = sprite->sectnum;
        entity->pal = sprite->pal;
        entity->cstat = 32768;
        VectorSet(entity->s.origin, sprite->x / 16.f, -sprite->y / 16.f, -sprite->z / 256.f);
        VectorCopy(entity->s.origin, entity->spawnorigin);
        entity->s.angles[1] = -sprite->ang * 360.f / 2048;
        DN_Link(entity);
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        edict_t *source = controllers[i];
        if (!source)
        {
            continue;
        }
        int target = dn_world.sprites[i].owner;
        source->owner = target >= 0 && target < dn_world.numsprites ? controllers[target] : NULL;
    }
    free(controllers);
}

static void TransportBeam(edict_t *source)
{
    edict_t *beam = DN_SpawnSprite(source, DN_ConSymbol("TRANSPORTERBEAM", 1261));
    DN_PlaySound(beam, DN_ConSymbol("TELEPORTER", 70), 0, 0);
}

static void PublishPlayer(edict_t *player)
{
    gclient_t *client = player->client;
    DN_PushMove(&dn_world, player->s.origin, player->mins, player->maxs, player->sector,
                MASK_PLAYERSOLID);
    for (int axis = 0; axis < 3; axis++)
    {
        client->ps.pmove.origin[axis] = (int32_t)lroundf(player->s.origin[axis] * 8);
        player->s.origin[axis] = client->ps.pmove.origin[axis] / 8.f;
    }
    client->ps.pmove.pm_flags &= ~PMF_ON_GROUND;
    VectorCopy(player->s.origin, player->s.old_origin);
    player->s.event = EV_PLAYER_TELEPORT;
    DN_Link(player);
}

int DN_CheckPlayerTransport(edict_t *player, const float *previous)
{
    gclient_t *client = player->client;
    if (!client || client->noclip)
    {
        return 0;
    }
    player->sector = DN_FindSector(&dn_world, player->s.origin, player->sector);
    dn_transport_query_t query = {};
    query.sector = player->sector;
    query.hold = client->transport_hold;
    query.jetpack = client->jetpack_on;
    query.grounded = (client->ps.pmove.pm_flags & PMF_ON_GROUND) != 0;
    query.jumping = client->ps.pmove.velocity[2] > 0;
    query.upmove = client->lastcmd.upmove;
    query.eyeheight = client->ps.viewoffset[2];
    VectorCopy(player->s.origin, query.point);
    query.has_previous = previous != NULL;
    if (previous)
    {
        VectorCopy(previous, query.previous);
    }
    dn_transport_result_t result;
    if (!DN_QueryPlayerTransport(&dn_world, &query, &result))
    {
        return 0;
    }
    edict_t *source = DN_MapSprite(result.source);
    edict_t *destination = DN_MapSprite(result.destination);
    if (result.onfloor)
    {
        for (int p = 1; p <= dn_maxclients; p++)
        {
            edict_t *other = &dn_edicts[p];
            if (other != player && other->inuse &&
                other->sector == dn_world.sprites[result.destination].sectnum)
            {
                other->health = other->client->health = 0;
            }
        }
        client->ps.viewangles[YAW] = result.yaw;
        client->ps.pmove.delta_angles[YAW] = ANGLE2SHORT(result.yaw) - client->lastcmd.angles[YAW];
        player->s.angles[YAW] = result.yaw;
        client->transport_hold = result.hold;
        if (source && destination)
        {
            source->transport_hold = destination->transport_hold = result.hold;
            if (!source->pal)
            {
                TransportBeam(source);
                TransportBeam(destination);
            }
        }
    }
    VectorCopy(result.point, player->s.origin);
    player->sector = dn_world.sprites[result.destination].sectnum;
    PublishPlayer(player);
    return 1;
}

void DN_TickTransports(void)
{
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        if (IsTransport(&dn_edicts[i]) && dn_edicts[i].transport_hold > 0)
        {
            dn_edicts[i].transport_hold--;
        }
    }
    for (int p = 1; p <= dn_maxclients; p++)
    {
        edict_t *player = &dn_edicts[p];
        if (!player->inuse)
        {
            continue;
        }
        if (player->client->transport_hold > 0)
        {
            if (--player->client->transport_hold == 0)
            {
                for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
                {
                    if (IsTransport(&dn_edicts[i]) && dn_edicts[i].sector == player->sector)
                    {
                        player->client->transport_hold = 2;
                        break;
                    }
                }
            }
        }
        DN_CheckPlayerTransport(player, NULL);
    }
}
