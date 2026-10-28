/* Native enemy spawn settings and movefta() visibility-gated activation. */
#include "dncon.h"

static int TileNamed(const edict_t *entity, const char *name)
{
    return entity->tile == DN_ConSymbol(name, -1);
}

static void RecognitionSound(edict_t *entity)
{
    if (entity->health <= 0)
    {
        return;
    }
    if (entity->health > 0 && entity->tile >= DN_ConSymbol("LIZTROOP", 1680) &&
        entity->tile <= DN_ConSymbol("LIZTROOPJUSTSIT", 1742) &&
        !TileNamed(entity, "LIZTROOPSTAYPUT"))
    {
        DN_PlaySound(entity, DN_ConSymbol("PRED_RECOG", 110), 0, 0);
    }

    static const struct
    {
        const char *tile, *sound;
    } sounds[] = {{"LIZMAN", "CAPT_RECOG"},
                  {"LIZMANSPITTING", "CAPT_RECOG"},
                  {"LIZMANFEEDING", "CAPT_RECOG"},
                  {"LIZMANJUMP", "CAPT_RECOG"},
                  {"PIGCOP", "PIG_RECOG"},
                  {"PIGCOPDIVE", "PIG_RECOG"},
                  {"RECON", "RECO_RECOG"},
                  {"DRONE", "DRON_RECOG"},
                  {"COMMANDER", "COMM_RECOG"},
                  {"COMMANDERSTAYPUT", "COMM_RECOG"},
                  {"ORGANTIC", "TURR_RECOG"},
                  {"OCTABRAIN", "OCTA_RECOG"},
                  {"OCTABRAINSTAYPUT", "OCTA_RECOG"},
                  {"GREENSLIME", "SLIM_RECOG"},
                  {"BOSS1", "BOS1_RECOG"}};

    for (int i = 0; i < sizeof(sounds) / sizeof(sounds[0]); i++)
    {
        if (TileNamed(entity, sounds[i].tile))
        {
            DN_PlaySound(entity, DN_ConSymbol(sounds[i].sound, -1), 0, TileNamed(entity, "BOSS1"));
        }
    }
    if (TileNamed(entity, "BOSS2") || TileNamed(entity, "BOSS3"))
    {
        const char *sound = TileNamed(entity, "BOSS2")
                                ? (entity->pal == 1 ? "BOS2_RECOG" : "WHIPYOURASS")
                                : (entity->pal == 1 ? "BOS3_RECOG" : "RIPHEADNECK");
        DN_PlaySound(entity, DN_ConSymbol(sound, -1), 0, 1);
    }
    if (TileNamed(entity, "BOSS4") || TileNamed(entity, "BOSS4STAYPUT"))
    {
        if (entity->pal == 1)
        {
            DN_PlaySound(entity, DN_ConSymbol("BOS4_RECOG", -1), 0, 1);
        }
        DN_PlaySound(entity, DN_ConSymbol("BOSS4_FIRSTSEE", -1), 0, 1);
    }
}

void DN_InitEnemy(edict_t *entity, qboolean map_spawn)
{
    DN_CountLevelEnemy(entity);
    if (!entity->enemy)
    {
        return;
    }
    if (TileNamed(entity, "EGG"))
    {
        entity->clipdist = 24;
        entity->cstat = 257 | (DN_Random() & 4);
        entity->dormant = map_spawn;
        return;
    }
    if (TileNamed(entity, "RECON"))
    {
        /* Native spawn keeps the authored size, altitude and clip distance.
           Recon is not initialized through the walking-enemy fall path. */
        entity->health = 130;
        entity->shade = -17;
        entity->pal = 0;
        entity->cstat |= 257;
        entity->native_state[4] = -1;
        entity->dormant = map_spawn;
        entity->alert_ticks = 0;
        if (!map_spawn)
        {
            RecognitionSound(entity);
        }
        return;
    }
    static const char *stayput[] = {"OCTABRAINSTAYPUT", "LIZTROOPSTAYPUT", "PIGCOPSTAYPUT",
                                    "LIZMANSTAYPUT",    "BOSS1STAYPUT",    "PIGCOPDIVE",
                                    "COMMANDERSTAYPUT", "BOSS4STAYPUT"};
    for (int i = 0; i < sizeof(stayput) / sizeof(stayput[0]); i++)
    {
        if (TileNamed(entity, stayput[i]))
        {
            entity->stayput = entity->sector;
        }
    }
    int troop = entity->tile >= DN_ConSymbol("LIZTROOP", 1680) &&
                entity->tile <= DN_ConSymbol("LIZTROOPJUSTSIT", 1742);
    if (troop && entity->pal == 0)
    {
        entity->pal = 22;
    }
    int boss = TileNamed(entity, "BOSS1") || TileNamed(entity, "BOSS1STAYPUT") ||
               TileNamed(entity, "BOSS2") || TileNamed(entity, "BOSS3") ||
               TileNamed(entity, "BOSS4") || TileNamed(entity, "BOSS4STAYPUT");
    int shark = TileNamed(entity, "SHARK");
    entity->xrepeat = entity->yrepeat = boss && !entity->pal ? 80 : shark ? 60 : 40;
    entity->clipdist = boss && !entity->pal ? 164 : shark ? 40 : 80;
    entity->cstat |= 257;
    if (TileNamed(entity, "GREENSLIME"))
    {
        entity->health = 1;
    }
    if (TileNamed(entity, "RAT"))
    {
        entity->xrepeat = entity->yrepeat = 48;
        entity->cstat = 0;
        entity->s.angles[YAW] = -(DN_Random() & 2047) * 360.f / 2048;
    }
    if (TileNamed(entity, "ORGANTIC"))
    {
        entity->cstat |= 128;
    }
    DN_Fall(entity);
    entity->dormant = map_spawn;
    entity->alert_ticks = 0;
    if (!map_spawn)
    {
        RecognitionSound(entity);
        edict_t *player = DN_NearestPlayer(entity, NULL);
        if (player)
        {
            VectorCopy(player->s.origin, entity->lastseen);
        }
    }
}

int DN_EnemyDormant(edict_t *entity)
{
    if (!entity->dormant)
    {
        return 0;
    }
    if (entity->sector >= 0 && entity->sector < dn_world.numsectors)
    {
        const dn_sector_t *sector = &dn_world.sectors[entity->sector];
        entity->shade = sector->ceilingstat & 1 ? sector->ceilingshade : sector->floorshade;
    }
    float distance;
    edict_t *player = DN_NearestPlayer(entity, &distance);
    if (!player || player->client->health <= 0 || distance >= 30000.f / 16)
    {
        return 1;
    }
    if (++entity->alert_ticks < ((int)(distance * 16) >> 8))
    {
        return 1;
    }
    vec3_t eye, source;
    VectorCopy(player->s.origin, eye);
    eye[0] += (64 - (DN_Random() & 127)) / 16.f;
    eye[1] -= (64 - (DN_Random() & 127)) / 16.f;
    int player_sector = DN_FindSector(&dn_world, eye, entity->sector);
    if (player_sector < 0)
    {
        return 1;
    }
    VectorCopy(entity->s.origin, source);
    source[0] += (64 - (DN_Random() & 127)) / 16.f;
    source[1] -= (64 - (DN_Random() & 127)) / 16.f;
    source[2] += (DN_Random() % (52 << 8)) / 256.f;
    eye[2] += player->client->ps.viewoffset[2] + (DN_Random() % (32 << 8)) / 256.f;
    int visible = DN_CanSee(&dn_world, source, entity->sector, eye, player->sector);
    entity->alert_ticks = 0;
    if (visible)
    {
        entity->dormant = 0;
        VectorCopy(player->s.origin, entity->lastseen);
        RecognitionSound(entity);
    }
    DN_Link(entity);
    return !visible;
}
