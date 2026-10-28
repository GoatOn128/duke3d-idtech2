/* Switch-controlled SE12 lighting from GAME.cpp, PREMAP.cpp and ACTORS.cpp. */
#include "dncon.h"

typedef struct
{
    int32_t effector, state, floorshade, ceilingshade;
} dn_light_t;

static dn_light_t *lights;

void DN_FreeLights(void)
{
    free(lights);
    lights = NULL;
}

void DN_InitLights(void)
{
    DN_FreeLights();
    lights = (dn_light_t *)(calloc(dn_world.numsprites, sizeof(*lights)));
    if (dn_world.numsprites && !lights)
    {
        gi.error("Cannot allocate Duke light controllers");
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        const dn_sprite_t *sprite = &dn_world.sprites[i];
        lights[i].effector = -1;
        if (sprite->picnum == DN_ConSymbol("SECTOREFFECTOR", 1) && sprite->lotag == 12)
        {
            lights[i].effector = i;
            lights[i].floorshade = dn_world.sectors[sprite->sectnum].floorshade;
            lights[i].ceilingshade = dn_world.sectors[sprite->sectnum].ceilingshade;
        }
    }
}

void DN_ActivateLights(int tag, int initialize)
{
    if (!tag)
    {
        return;
    }
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        dn_light_t *light = &lights[i];
        if (light->effector < 0 || dn_world.sprites[i].hitag != tag)
        {
            continue;
        }
        /* Native states: 0 off, 1 flickering on, 2 on, 3 restoring darkness. */
        light->state = initialize ? 1 : light->state + 1;
        if (light->state == 2)
        {
            light->state = 3;
        }
        dn_world.sectors[dn_world.sprites[i].sectnum].floorpal = 0;
        DN_PublishSectorStyle(dn_world.sprites[i].sectnum);
    }
}

void DN_TickLights(void)
{
    for (int i = dn_world.numsprites - 1; i >= 0; i--)
    {
        dn_light_t *light = &lights[i];
        if (light->effector < 0 || (light->state != 1 && light->state != 3))
        {
            continue;
        }
        const dn_sprite_t *effector = &dn_world.sprites[i];
        int index = effector->sectnum;
        dn_sector_t *sector = &dn_world.sectors[index];
        int restore = light->state == 3;
        if (!restore && sector->floorshade <= effector->shade)
        {
            light->state = 2;
        }
        else
        {
            sector->floorpal = sector->ceilingpal = restore ? 0 : effector->pal;
            sector->floorshade = restore ? light->floorshade : sector->floorshade - 2;
            sector->ceilingshade = restore ? light->ceilingshade : sector->ceilingshade - 2;
            for (int wall = sector->wallptr; wall < sector->wallptr + sector->wallnum; wall++)
            {
                dn_wall_t *side = &dn_world.walls[wall];
                if (side->hitag != 1)
                {
                    side->pal = restore ? 0 : effector->pal;
                    side->shade = restore ? light->floorshade : side->shade - 2;
                    DN_PublishWallStyle(wall);
                }
            }
            DN_PublishSector(index);
            DN_PublishSectorStyle(index);
        }
        for (int entity = dn_maxclients + 1; entity < globals.num_edicts; entity++)
        {
            edict_t *sprite = &dn_edicts[entity];
            if (sprite->inuse && !(sprite->s.renderfx & RF_DUKE_WORLD) && sprite->sector == index &&
                (sprite->cstat & 16))
            {
                sprite->shade =
                    (sector->ceilingstat & 1) ? sector->ceilingshade : sector->floorshade;
                DN_Link(sprite);
            }
        }
        if (restore)
        {
            light->state = 0;
        }
    }
}

void DN_WriteLightState(FILE *file)
{
    DN_WriteSaveData(file, lights, sizeof(*lights), dn_world.numsprites);
}

void DN_ReadLightState(FILE *file)
{
    DN_ReadSaveData(file, lights, sizeof(*lights), dn_world.numsprites);
    for (int i = 0; i < dn_world.numsprites; i++)
    {
        const dn_light_t *light = &lights[i];
        const dn_sprite_t *sprite = &dn_world.sprites[i];
        if ((light->effector != -1 && light->effector != i) || light->state < 0 ||
            light->state > 3 || light->floorshade < -128 || light->floorshade > 127 ||
            light->ceilingshade < -128 || light->ceilingshade > 127 ||
            (light->effector >= 0 &&
             (sprite->picnum != DN_ConSymbol("SECTOREFFECTOR", 1) || sprite->lotag != 12)))
        {
            fclose(file);
            gi.error("Invalid saved light controller %d", i);
        }
    }
}
