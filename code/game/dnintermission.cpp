/* Authoritative level results, frozen intermission and explicit continuation. */
#include "dncon.h"

static struct
{
    int32_t active, ticks, queued, destination, enemies, secrets;
} results;

int DN_IntermissionActive(void)
{
    return results.active;
}

void DN_InitLevelResults(void)
{
    memset(&results, 0, sizeof(results));
    for (int i = 0; i < dn_world.numsectors; i++)
    {
        results.secrets += dn_world.sectors[i].lotag == 32767;
    }
    gi.configstring(DN_CS_INTERMISSION, "");
    gi.configstring(DN_CS_LEVEL_NAME, "");
}

void DN_CountLevelEnemy(edict_t *entity)
{
    if (entity->enemy && entity->tile != DN_ConSymbol("RAT", 1267) &&
        entity->tile != DN_ConSymbol("SHARK", 1550) && entity->tile != DN_ConSymbol("EGG", 675) &&
        results.enemies < INT16_MAX)
    {
        results.enemies++;
    }
}

static void PublishResults(void)
{
    int episode = 0, level = 0;
    char text[MAX_QPATH];
    sscanf(dn_current_map, "e%dl%d", &episode, &level);
    const dn_level_info_t *info = DN_ConLevel(episode, level);
    snprintf(text, sizeof(text), "%d %d %d %d %d %d", episode, (int)dn_time,
             info ? info->par_seconds : 0, info ? info->designer_seconds : 0,
             gi.cvar("skill", "1", 0)->value >= 3 ? -1 : results.enemies, results.secrets);
    char name[MAX_QPATH];
    snprintf(name, sizeof(name), "%s", info && info->name[0] ? info->name : dn_current_map);
    gi.configstring(DN_CS_LEVEL_NAME, name);
    gi.configstring(DN_CS_INTERMISSION, text);
}

void DN_SetIntermissionStats(edict_t *player)
{
    gclient_t *client = player->client;
    client->ps.stats[DN_STAT_FORMAT] = DN_PLAYER_FORMAT;
    client->ps.stats[DN_STAT_INTERMISSION] = 1;
    client->ps.stats[STAT_FRAGS] = client->kills;
    client->ps.stats[DN_STAT_SECRETS] = client->secrets;
    client->ps.pmove.pm_type = PM_FREEZE;
    client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
    VectorClear(client->ps.pmove.velocity);
    VectorClear(client->ps.blend);
    client->ps.gunindex = 0;
}

void DN_BeginIntermission(int destination)
{
    if (results.active)
    {
        return;
    }
    results.active = 1;
    results.destination = destination >= 1 && destination <= 11 ? destination : 0;
    gi.configstring(CS_CDTRACK, "");
    PublishResults();
    for (int i = 1; i <= dn_maxclients; i++)
    {
        if (dn_edicts[i].inuse && dn_edicts[i].client)
        {
            DN_UpdatePlayerStats(&dn_edicts[i]);
        }
    }
    DN_PlaySound(&dn_edicts[1], DN_ConSymbol("BONUSMUSIC", 249), 0, 1);
    gi.dprintf("Duke level completed: %s time %d enemies %d secrets %d\n", dn_current_map,
               (int)dn_time, results.enemies, results.secrets);
}

void DN_CheckLevelSector(edict_t *player)
{
    if (!player->client || player->client->health <= 0 || results.active || player->sector < 0 ||
        player->sector >= dn_world.numsectors)
    {
        return;
    }
    dn_sector_t *sector = &dn_world.sectors[player->sector];
    if (sector->lotag == 32767)
    {
        sector->lotag = 0;
        player->client->secrets++;
        gi.centerprintf(player, "%s", dn_quotes[9]);
    }
    else if (sector->lotag == -1)
    {
        DN_BeginIntermission(0);
    }
    else if (sector->lotag >= 10000 && sector->lotag < 16383)
    {
        /* Native checksectors: a one-shot sound, including E1L1's opening line. */
        DN_PlaySound(player, sector->lotag - 10000, 0, 0);
        sector->lotag = 0;
    }
}

void DN_IntermissionFrame(void)
{
    if (results.ticks < 10)
    {
        results.ticks++;
    }
}

void DN_ContinueIntermission(void)
{
    char name[MAX_QPATH], path[MAX_QPATH + 16], command[96];
    int episode = 1, level = 1;
    if (!results.active || results.ticks < 10 || results.queued)
    {
        gi.dprintf("Duke results continue deferred: active %d ticks %d queued %d\n", results.active,
                   results.ticks, results.queued);
        return;
    }
    sscanf(dn_current_map, "e%dl%d", &episode, &level);
    level = results.destination ? results.destination : level + 1;
    snprintf(name, sizeof(name), "e%dl%d", episode, level);
    snprintf(path, sizeof(path), "maps/%s.map", name);
    void *data = DN_ReadAsset(path, NULL);
    if (!data)
    {
        /* Never leave the player on a missing-map error at an episode boundary. */
        snprintf(name, sizeof(name), "e%dl1", episode);
    }
    free(data);
    results.queued = 1;
    gi.dprintf("Duke results continue: %s\n", name);
    snprintf(command, sizeof(command), "gamemap %s\n", name);
    gi.AddCommandString(command);
}

void DN_WriteLevelResults(FILE *file)
{
    DN_WriteSaveData(file, &results, sizeof(results), 1);
}

void DN_ReadLevelResults(FILE *file)
{
    DN_ReadSaveData(file, &results, sizeof(results), 1);
    if (results.active < 0 || results.active > 1 || results.ticks < 0 || results.ticks > 10 ||
        results.queued < 0 || results.queued > 1 || results.destination < 0 ||
        results.destination > 11 || results.enemies < 0 || results.enemies > INT16_MAX ||
        results.secrets < 0 || results.secrets > dn_world.numsectors)
    {
        fclose(file);
        gi.error("Invalid saved level results");
    }
    results.queued = 0;
    gi.configstring(DN_CS_INTERMISSION, "");
    gi.configstring(DN_CS_LEVEL_NAME, "");
    if (results.active)
    {
        PublishResults();
        for (int i = 1; i <= dn_maxclients; i++)
        {
            if (dn_edicts[i].inuse && dn_edicts[i].client)
            {
                DN_SetIntermissionStats(&dn_edicts[i]);
            }
        }
    }
}
