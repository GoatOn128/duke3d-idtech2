/* Discrete wall/plane styles travel in acknowledged world snapshots. */
#include "dnlocal.h"

static edict_t **style_markers;
static edict_t **sector_styles;

void DN_InitStyles(void)
{
    free(style_markers);
    style_markers = (edict_t **)(calloc(dn_world.numwalls, sizeof(*style_markers)));
    free(sector_styles);
    sector_styles = (edict_t **)(calloc(dn_world.numsectors, sizeof(*sector_styles)));
    if (!style_markers || !sector_styles)
    {
        gi.error("Cannot allocate Duke world styles");
    }
}

void DN_FreeStyles(void)
{
    free(style_markers);
    style_markers = NULL;
    free(sector_styles);
    sector_styles = NULL;
}

void DN_PublishWallStyle(int index)
{
    dn_wall_t *wall;
    edict_t *entity;
    if (index < 0 || index >= dn_world.numwalls)
    {
        return;
    }
    wall = &dn_world.walls[index];
    entity = style_markers[index];
    if (!entity)
    {
        entity = style_markers[index] = DN_NewEntity();
        entity->s.renderfx = RF_DUKE_WALL_STYLE;
        entity->s.modelindex = dn_sprite_model;
        entity->s.frame = index;
    }
    entity->s.skinnum = (int32_t)((uint16_t)wall->picnum | ((uint32_t)wall->cstat << 16));
    entity->s.effects = (uint16_t)wall->overpicnum | ((uint32_t)(uint8_t)wall->shade << 16) |
                        ((uint32_t)wall->pal << 24);
    DN_Link(entity);
}

void DN_PublishSectorStyle(int index)
{
    if (index < 0 || index >= dn_world.numsectors)
    {
        return;
    }
    const dn_sector_t *sector = &dn_world.sectors[index];
    edict_t *entity = sector_styles[index];
    if (!entity)
    {
        entity = sector_styles[index] = DN_NewEntity();
        entity->s.renderfx = RF_DUKE_SECTOR_STYLE;
        entity->s.modelindex = dn_sprite_model;
        entity->s.frame = index;
    }
    entity->s.skinnum = (int32_t)((uint16_t)sector->ceilingpicnum |
                                  ((uint32_t)(uint16_t)sector->floorpicnum << 16));
    entity->s.effects = sector->ceilingstat | ((uint32_t)sector->floorstat << 16);
    entity->s.angles[0] = sector->ceilingpal * 360.f / 256;
    entity->s.angles[1] = sector->floorpal * 360.f / 256;
    entity->s.angles[2] = sector->ceilingxpanning * 360.f / 256;
    VectorSet(entity->s.origin, sector->ceilingypanning, sector->floorxpanning,
              sector->floorypanning);
    DN_Link(entity);
}

int DN_RestoreStyle(edict_t *entity)
{
    int index = entity->s.frame;
    if (entity->s.renderfx == RF_DUKE_SECTOR_STYLE)
    {
        if (index < 0 || index >= dn_world.numsectors || sector_styles[index])
        {
            return 0;
        }
        sector_styles[index] = entity;
        return 1;
    }
    if (index < 0 || index >= dn_world.numwalls || style_markers[index])
    {
        return 0;
    }
    style_markers[index] = entity;
    return 1;
}
