/* Native touchplates and SE31 true-drop floors, including the E1L1 RPG lift. */
#include "dncon.h"

typedef struct
{
    int32_t type, original, active, retract, delay;
    int32_t occupied, exhausted;
} dn_drop_state_t;

static dn_drop_state_t *states;

void DN_FreeDropFloors(void)
{
    free(states);
    states = NULL;
}

void DN_InitDropFloors(void)
{
    DN_FreeDropFloors();
    states = (dn_drop_state_t *)(calloc(dn_world.numsprites, sizeof(*states)));
    if (dn_world.numsprites && !states)
    {
        gi.error("Cannot allocate Duke floor controllers");
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        dn_sprite_t *sprite = &dn_world.sprites[i];
        dn_sector_t *sector = &dn_world.sectors[sprite->sectnum];
        dn_drop_state_t *state = &states[i];
        if (sprite->picnum == DN_ConSymbol("SECTOREFFECTOR", 1) && sprite->lotag == 31)
        {
            state->type = 31;
            state->original = sector->floorz;
            sprite->yvel = sector->extra;
            if (sprite->ang != 1536)
            {
                sector->floorz = sprite->z;
            }
            for (int wall = sector->wallptr; wall < sector->wallptr + sector->wallnum; wall++)
            {
                if (!dn_world.walls[wall].hitag)
                {
                    dn_world.walls[wall].hitag = 9999;
                }
            }
            DN_PublishSector(sprite->sectnum);
        }
        else if (sprite->picnum == DN_ConSymbol("TOUCHPLATE", 3))
        {
            state->type = sprite->pal && dn_maxclients > 1 ? 0 : 3;
            state->original = sector->floorz;
            if (sector->lotag != 1 && sector->lotag != 2)
            {
                sector->floorz = sprite->z;
                DN_PublishSector(sprite->sectnum);
            }
        }
    }
}

int DN_DropFloorMoving(int sector)
{
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        if (states[i].type == 31 && states[i].active && dn_world.sprites[i].sectnum == sector)
        {
            return 1;
        }
    }
    return 0;
}

int DN_ActivateDropFloor(int sector)
{
    int found = 0;
    if (dn_world.sectors[sector].lotag >= 3)
    {
        return 0;
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        if (states[i].type == 31 && dn_world.sprites[i].sectnum == sector)
        {
            states[i].active ^= 1;
            found = 1;
        }
    }
    return found;
}

static void CarryFloor(int sector, int32_t delta, int plate)
{
    float movement = -delta / 256.f;
    for (int i = 1; i < globals.num_edicts; i++)
    {
        edict_t *entity = &dn_edicts[i];
        if (!entity->inuse || entity->sector != sector ||
            (entity->s.renderfx & (RF_DUKE_WORLD | RF_BEAM)) || entity->projectile ||
            (entity->client ? !plate && !(entity->client->ps.pmove.pm_flags & PMF_ON_GROUND)
                            : entity->zspeed != 0))
        {
            continue;
        }
        entity->s.origin[2] += movement;
        if (entity->client)
        {
            entity->client->ps.pmove.origin[2] = lroundf(entity->s.origin[2] * 8);
            entity->client->ps.prediction.motion_z += delta;
        }
        DN_Link(entity);
    }
}

void DN_TickDropFloors(void)
{
    for (int i = dn_world.numsprites - 1; i >= 0; i--)
    {
        dn_drop_state_t *state = &states[i];
        if (state->type != 31 || !state->active)
        {
            continue;
        }
        dn_sprite_t *sprite = &dn_world.sprites[i];
        dn_sector_t *sector = &dn_world.sectors[sprite->sectnum];
        if (state->delay > 0)
        {
            state->delay--;
            continue;
        }
        int target = ((sprite->ang == 1536) != state->retract) ? sprite->z : state->original;
        int speed = DN_Max(1, sprite->yvel);
        int64_t distance = (int64_t)target - sector->floorz;
        if (llabs(distance) < speed)
        {
            if (state->retract)
            {
                sector->floorz = target;
            }
            state->active = 0;
            state->retract ^= 1;
            state->delay = sprite->hitag;
        }
        else
        {
            int delta = distance > 0 ? speed : -speed;
            sector->floorz += delta;
            CarryFloor(sprite->sectnum, delta, 0);
        }
        DN_PublishSector(sprite->sectnum);
    }
}

void DN_TickTouchplates(void)
{
    for (int i = dn_world.numsprites - 1; i >= 0; i--)
    {
        dn_drop_state_t *state = &states[i];
        if (state->type != 3)
        {
            continue;
        }
        dn_sprite_t *sprite = &dn_world.sprites[i];
        dn_sector_t *sector = &dn_world.sectors[sprite->sectnum];
        if (state->active && sprite->hitag >= 0)
        {
            int target = state->retract ? state->original : sprite->z;
            int speed = DN_Max(1, sector->extra);
            int delta = DN_Max(-speed, DN_Min(speed, target - sector->floorz));
            sector->floorz += delta;
            if (!delta)
            {
                state->active = 0;
            }
            else
            {
                CarryFloor(sprite->sectnum, delta, 1);
                DN_PublishSector(sprite->sectnum);
            }
            continue;
        }
        if (state->exhausted)
        {
            continue;
        }
        edict_t *player = NULL;
        for (int p = 1; p <= dn_maxclients; p++)
        {
            if (dn_edicts[p].inuse && dn_edicts[p].client &&
                dn_edicts[p].sector == sprite->sectnum &&
                ((dn_clients[p - 1].ps.pmove.pm_flags & PMF_ON_GROUND) || sprite->ang == 512))
            {
                player = &dn_edicts[p];
                break;
            }
        }
        if (!player)
        {
            state->occupied = 0;
            continue;
        }
        if (state->occupied)
        {
            continue;
        }
        int moving = 0;
        for (int controller = 0; controller < dn_world.numsprites; controller++)
        {
            const dn_sprite_t *activator = &dn_world.sprites[controller];
            if ((activator->picnum == 2 || activator->picnum == 4) &&
                activator->lotag == sprite->lotag && DN_SectorMoving(activator->sectnum))
            {
                moving = 1;
                break;
            }
        }
        if (moving)
        {
            continue;
        }
        state->occupied = state->active = 1;
        state->retract ^= 1;
        DN_ActivateTag(sprite->lotag, player);
        if (sprite->hitag > 0 && --sprite->hitag == 0)
        {
            state->exhausted = 1;
        }
        for (int other = 0; other < dn_world.numsprites; other++)
        {
            if (other != i && states[other].type == 3 &&
                dn_world.sprites[other].lotag == sprite->lotag)
            {
                states[other].active = 1;
                states[other].retract = state->retract;
            }
        }
    }
}

void DN_WriteDropState(FILE *file)
{
    DN_WriteSaveData(file, states, sizeof(*states), dn_world.numsprites);
}

void DN_ReadDropState(FILE *file)
{
    DN_ReadSaveData(file, states, sizeof(*states), dn_world.numsprites);
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        const dn_drop_state_t *state = &states[i];
        const dn_sprite_t *sprite = &dn_world.sprites[i];
        if ((state->type != 0 && state->type != 3 && state->type != 31) || (state->active & ~1) ||
            (state->retract & ~1) || (state->occupied & ~1) || (state->exhausted & ~1) ||
            (state->type == 3 && sprite->picnum != DN_ConSymbol("TOUCHPLATE", 3)) ||
            (state->type == 31 && (sprite->picnum != 1 || sprite->lotag != 31)))
        {
            fclose(file);
            gi.error("Invalid saved floor controller %d", i);
        }
    }
}
