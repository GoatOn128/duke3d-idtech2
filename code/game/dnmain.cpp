/* Game DLL entry point, lifecycle and simulation clock. */
#include "dnlocal.h"
#include "dncon.h"

game_import_t gi;
game_export_t globals;
edict_t *dn_edicts;
gclient_t *dn_clients;
dn_map_t dn_world;
float dn_time;
int dn_maxclients, dn_sprite_model;
static int dn_frame_number;
char dn_current_map[MAX_QPATH];

static void InitGame(void)
{
    gi.dprintf("==== Duke3D game: runtime CON, Quake client/server ====\n");
    dn_maxclients = (int)gi.cvar("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH)->value;
    if (dn_maxclients < 1 || dn_maxclients > DN_MAX_CLIENTS)
    {
        gi.error("Duke supports 1 to 16 clients");
    }
    dn_edicts = (edict_t *)(gi.TagMalloc(sizeof(edict_t) * MAX_EDICTS, 765));
    memset(dn_edicts, 0, sizeof(edict_t) * MAX_EDICTS);
    dn_clients = (gclient_t *)(gi.TagMalloc(sizeof(gclient_t) * dn_maxclients, 765));
    memset(dn_clients, 0, sizeof(gclient_t) * dn_maxclients);
    globals.edicts = dn_edicts;
    globals.edict_size = sizeof(edict_t);
    globals.max_edicts = MAX_EDICTS;
    globals.num_edicts = dn_maxclients + 1;
    for (int i = 1; i <= dn_maxclients; i++)
    {
        dn_edicts[i].client = &dn_clients[i - 1];
    }
    dn_con_import_t con_imports = {DN_ReadAsset, gi.dprintf, gi.error};
    DN_ConConfigure(&con_imports);
    if (!DN_ConLoad())
    {
        gi.error("Runtime CON compilation failed; see console");
    }
    DN_InitTiles();
    gi.cvar("skill", "1", CVAR_LATCH);
    gi.cvar("deathmatch", "0", CVAR_LATCH);
    gi.cvar("coop", "0", CVAR_LATCH);
    gi.cvar("sv_gravity", "800", 0);
    gi.cvar("respawn_monsters", "0", 0);
    gi.cvar("respawn_items", "0", 0);
}

static void ShutdownGame(void)
{
    DN_ConFree();
    DN_FreeMap(&dn_world);
    DN_FreeSectors();
    gi.FreeTags(765);
    dn_edicts = NULL;
}

static void RunFrame(void)
{
    if (DN_IntermissionActive())
    {
        DN_IntermissionFrame();
        return;
    }
    ++dn_frame_number;
    for (int step = 1; step <= DN_SIMULATION_HZ / 10; step++)
    {
        dn_time =
            ((dn_frame_number - 1) * (DN_SIMULATION_HZ / 10) + step) / (float)DN_SIMULATION_HZ;
        DN_TickFallers();
        DN_MoveSectors();
        DN_TickInteractions();
        DN_TickTransports();
        int count = globals.num_edicts;
        for (int i = 1; i < count; i++)
        {
            if (dn_edicts[i].inuse && !(dn_edicts[i].s.renderfx & RF_DUKE_WORLD))
            {
                if (dn_edicts[i].client)
                {
                    DN_WeaponTick(&dn_edicts[i]);
                }
                DN_ActorTick(&dn_edicts[i]);
                if (DN_IntermissionActive())
                {
                    return;
                }
            }
        }
        DN_TickDestructibles();
        for (int player = 1; player <= dn_maxclients; player++)
        {
            if (dn_edicts[player].inuse)
            {
                DN_UpdatePlayerStats(&dn_edicts[player]);
            }
        }
    }
}

void DN_ResetClock(float time)
{
    dn_frame_number = (int)lroundf(time * 10);
    dn_time = time;
}

void Com_Printf(char *fmt, ...)
{
    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    gi.dprintf("%s", message);
}

extern "C" __declspec(dllexport) game_export_t *GetGameAPI(game_import_t *import)
{
    gi = *import;
    memset(&globals, 0, sizeof(globals));
    globals.apiversion = GAME_API_VERSION;
    globals.Init = InitGame;
    globals.Shutdown = ShutdownGame;
    globals.SpawnEntities = DN_SpawnEntities;
    globals.WriteGame = DN_WriteGame;
    globals.ReadGame = DN_ReadGame;
    globals.WriteLevel = DN_WriteLevel;
    globals.ReadLevel = DN_ReadLevel;
    globals.ClientConnect = DN_ClientConnect;
    globals.ClientBegin = DN_ClientBegin;
    globals.ClientUserinfoChanged = DN_ClientUserinfoChanged;
    globals.ClientDisconnect = DN_ClientDisconnect;
    globals.ClientCommand = DN_ClientCommand;
    globals.ClientThink = DN_ClientThink;
    globals.RunFrame = RunFrame;
    globals.ServerCommand = DN_ServerCommand;
    globals.edict_size = sizeof(edict_t);
    return &globals;
}
