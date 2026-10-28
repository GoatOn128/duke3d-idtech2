/* Player colors and the replicated roster for the original deathmatch frag bar. */
#include "dnlocal.h"

static char scores[DN_SCORE_PLAYERS][MAX_QPATH];

int DN_PlayerPalette(const edict_t *player)
{
    /* PREMAP.cpp assigns palettes 9..16, wrapping for the next eight slots. */
    return dn_maxclients > 1 || DN_IsDeathmatch() ? 9 + ((int)(player - dn_edicts) - 1) % 8 : 0;
}

void DN_InitMultiplayer(void)
{
    char clients[16];
    snprintf(clients, sizeof(clients), "%d", dn_maxclients);
    gi.configstring(CS_MAXCLIENTS, clients);
    memset(scores, 0, sizeof(scores));
    gi.configstring(DN_CS_DEATHMATCH, (char *)(DN_IsDeathmatch() ? "1" : ""));
    for (int i = 0; i < DN_SCORE_PLAYERS; i++)
    {
        gi.configstring(DN_CS_PLAYER_SCORES + i, "");
        if (i < dn_maxclients)
        {
            dn_clients[i].frags = 0;
        }
    }
}

void DN_PublishPlayerScore(const edict_t *player)
{
    int slot = (int)(player - dn_edicts) - 1;
    if (slot < 0 || slot >= dn_maxclients || slot >= DN_SCORE_PLAYERS)
    {
        return;
    }
    char text[MAX_QPATH] = {};
    if (DN_IsDeathmatch() && player->inuse && player->client && player->client->connected)
    {
        char name[sizeof(player->client->name)];
        snprintf(name, sizeof(name), "%s", player->client->name);
        for (int i = 0; name[i]; i++)
        {
            if ((unsigned char)name[i] < 32 || (unsigned char)name[i] > 126)
            {
                name[i] = '?';
            }
        }
        snprintf(text, sizeof(text), "%d %d %s", DN_PlayerPalette(player), player->client->frags,
                 name[0] ? name : "DUKE");
    }
    if (strcmp(text, scores[slot]))
    {
        snprintf(scores[slot], sizeof(scores[slot]), "%s", text);
        gi.configstring(DN_CS_PLAYER_SCORES + slot, text);
    }
}

void DN_PlayerFrag(edict_t *victim, edict_t *attacker)
{
    if (!DN_IsDeathmatch())
    {
        return;
    }
    edict_t *scorer = attacker && attacker != victim && attacker->client ? attacker : victim;
    int change = scorer == victim ? -1 : 1;
    scorer->client->frags = DN_Max(INT16_MIN, DN_Min(INT16_MAX, scorer->client->frags + change));
    DN_PublishPlayerScore(scorer);
}
