/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "server.h"
#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <sys/stat.h>
#include <SDL3/SDL_timer.h>

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
void SV_SetMaster_f(void)
{
    int i, slot;

    // only dedicated servers send heartbeats
    if (!dedicated->value)
    {
        Com_Printf("Only dedicated servers use masters.\n");
        return;
    }

    // make sure the server is listed public
    Cvar_Set("public", "1");

    for (i = 1; i < MAX_MASTERS; i++)
    {
        memset(&master_adr[i], 0, sizeof(master_adr[i]));
    }

    slot = 1; // slot 0 will always contain the id master
    for (i = 1; i < Cmd_Argc(); i++)
    {
        if (slot == MAX_MASTERS)
        {
            break;
        }

        if (!NET_StringToAdr(Cmd_Argv(i), &master_adr[i]))
        {
            Com_Printf("Bad address: %s\n", Cmd_Argv(i));
            continue;
        }
        if (master_adr[slot].port == 0)
        {
            master_adr[slot].port = BigShort(PORT_MASTER);
        }

        Com_Printf("Master server at %s\n", NET_AdrToString(master_adr[slot]));

        Com_Printf("Sending a ping.\n");

        Netchan_OutOfBandPrint(NS_SERVER, master_adr[slot], "ping");

        slot++;
    }

    svs.last_heartbeat = -9999999;
}

/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer(void)
{
    client_t *cl;
    int i;
    int idnum;
    char *s;

    if (Cmd_Argc() < 2)
    {
        return false;
    }

    s = Cmd_Argv(1);

    // numeric values are just slot numbers
    if (s[0] >= '0' && s[0] <= '9')
    {
        idnum = atoi(Cmd_Argv(1));
        if (idnum < 0 || idnum >= maxclients.GetFloat())
        {
            Com_Printf("Bad client slot: %i\n", idnum);
            return false;
        }

        sv_client = &svs.clients[idnum];
        sv_player = sv_client->edict;
        if (!sv_client->state)
        {
            Com_Printf("Client %i is not active\n", idnum);
            return false;
        }
        return true;
    }

    // check for a name match
    for (i = 0, cl = svs.clients; i < maxclients.GetFloat(); i++, cl++)
    {
        if (!cl->state)
        {
            continue;
        }
        if (!strcmp(cl->name, s))
        {
            sv_client = cl;
            sv_player = sv_client->edict;
            return true;
        }
    }

    Com_Printf("Userid %s is not on the server\n", s);
    return false;
}

/*
===============================================================================

SAVEGAME FILES

===============================================================================
*/

/*
=====================
SV_WipeSavegame

Delete save/<XXX>/
=====================
*/
void SV_WipeSavegame(char *savename)
{
    char name[MAX_OSPATH];
    char *s;

    Com_DPrintf("SV_WipeSaveGame(%s)\n", savename);

    Com_sprintf(name, sizeof(name), "%s/save/%s/server.ssv", FS_Gamedir(), savename);
    remove(name);
    Com_sprintf(name, sizeof(name), "%s/save/%s/game.ssv", FS_Gamedir(), savename);
    remove(name);

    Com_sprintf(name, sizeof(name), "%s/save/%s/*.sav", FS_Gamedir(), savename);
    s = Sys_FindFirst(name, 0, 0);
    while (s)
    {
        remove(s);
        s = Sys_FindNext(0, 0);
    }
    Sys_FindClose();
    Com_sprintf(name, sizeof(name), "%s/save/%s/*.sv2", FS_Gamedir(), savename);
    s = Sys_FindFirst(name, 0, 0);
    while (s)
    {
        remove(s);
        s = Sys_FindNext(0, 0);
    }
    Sys_FindClose();
}

/*
================
CopyFile
================
*/
static void CopySaveFile(const char *src, const char *dst, qboolean scanning)
{
    FILE *f1, *f2;
    int l;
    byte buffer[65536];

    Com_DPrintf("CopyFile (%s, %s)\n", src, dst);

    f1 = fopen(src, "rb");
    if (!f1)
    {
        if (scanning)
        {
            Sys_FindClose();
        }
        Com_Error(ERR_DROP, "Cannot read save file %s", src);
    }
    f2 = fopen(dst, "wb");
    if (!f2)
    {
        fclose(f1);
        if (scanning)
        {
            Sys_FindClose();
        }
        Com_Error(ERR_DROP, "Cannot write save file %s", dst);
    }

    while (1)
    {
        l = fread(buffer, 1, sizeof(buffer), f1);
        if (!l)
        {
            break;
        }
        if (fwrite(buffer, 1, l, f2) != l)
        {
            fclose(f1);
            fclose(f2);
            if (scanning)
            {
                Sys_FindClose();
            }
            Com_Error(ERR_DROP, "Failed copying save file %s", dst);
        }
    }

    int failed = ferror(f1);
    fclose(f1);
    failed |= fclose(f2) != 0;
    if (failed)
    {
        if (scanning)
        {
            Sys_FindClose();
        }
        Com_Error(ERR_DROP, "Failed copying save file %s", dst);
    }
}

static void SV_WriteSaveData(FILE *file, const void *data, size_t size)
{
    if (fwrite(data, 1, size, file) != size)
    {
        fclose(file);
        Com_Error(ERR_DROP, "Failed writing server save");
    }
}

static void SV_CloseSave(FILE *file)
{
    if (fclose(file))
    {
        Com_Error(ERR_DROP, "Failed closing server save");
    }
}

/*
================
RenameSaveDir

Windows refuses to rename a directory while any process holds a handle
inside it, and antivirus or search indexing only hold those handles for a
moment. Retry briefly before giving up, so a passing scan does not abort a
map load or a save.
================
*/
static qboolean RenameSaveDir(const char *src, const char *dst)
{
    for (int attempt = 0; attempt < 10; attempt++)
    {
        if (!rename(src, dst))
        {
            return true;
        }
        SDL_Delay(50);
    }
    return false;
}

/*
================
SV_CopySaveGame
================
*/
static void SV_CopySaveFiles(char *src, char *dst)
{
    char name[MAX_OSPATH], name2[MAX_OSPATH];
    int l, len;
    char *found;

    Com_DPrintf("SV_CopySaveGame(%s, %s)\n", src, dst);

    SV_WipeSavegame(dst);

    // copy the savegame over
    Com_sprintf(name, sizeof(name), "%s/save/%s/server.ssv", FS_Gamedir(), src);
    Com_sprintf(name2, sizeof(name2), "%s/save/%s/server.ssv", FS_Gamedir(), dst);
    FS_CreatePath(name2);
    CopySaveFile(name, name2, false);

    Com_sprintf(name, sizeof(name), "%s/save/%s/game.ssv", FS_Gamedir(), src);
    Com_sprintf(name2, sizeof(name2), "%s/save/%s/game.ssv", FS_Gamedir(), dst);
    CopySaveFile(name, name2, false);

    Com_sprintf(name, sizeof(name), "%s/save/%s/", FS_Gamedir(), src);
    len = strlen(name);
    Com_sprintf(name, sizeof(name), "%s/save/%s/*.sav", FS_Gamedir(), src);
    found = Sys_FindFirst(name, 0, 0);
    while (found)
    {
        strcpy(name + len, found + len);

        Com_sprintf(name2, sizeof(name2), "%s/save/%s/%s", FS_Gamedir(), dst, found + len);
        CopySaveFile(name, name2, true);

        // change sav to sv2
        l = strlen(name);
        strcpy(name + l - 3, "sv2");
        l = strlen(name2);
        strcpy(name2 + l - 3, "sv2");
        CopySaveFile(name, name2, true);

        found = Sys_FindNext(0, 0);
    }
    Sys_FindClose();
}

void SV_CopySaveGame(char *src, char *dst)
{
    static unsigned serial;
    char stage[64], stagepath[MAX_OSPATH], targetpath[MAX_OSPATH], backup[MAX_OSPATH];
    struct _stat info;
    qboolean existing;
    int attempts = 0;

    /* Copy every file before replacing the slot. In particular, a short write
       or missing source must not erase the player's previous good save. Names
       beginning with a dot cannot be supplied through save/load commands. */
    do
    {
        if (++attempts > 16)
        {
            Com_Error(ERR_DROP, "Cannot allocate a temporary save directory");
        }
        Com_sprintf(stage, sizeof(stage), ".dn-write-%08x-%u", (unsigned)Sys_Milliseconds(),
                    ++serial);
        Com_sprintf(stagepath, sizeof(stagepath), "%s/save/%s", FS_Gamedir(), stage);
        FS_CreatePath(stagepath);
        if (!_mkdir(stagepath))
        {
            break;
        }
        if (errno != EEXIST)
        {
            Com_Error(ERR_DROP, "Cannot create temporary save directory %s", stagepath);
        }
    } while (true);

    SV_CopySaveFiles(src, stage);
    Com_sprintf(targetpath, sizeof(targetpath), "%s/save/%s", FS_Gamedir(), dst);
    Com_sprintf(backup, sizeof(backup), "%s.old", stagepath);
    existing = _stat(targetpath, &info) == 0;
    if (existing && !RenameSaveDir(targetpath, backup))
    {
        Com_Error(ERR_DROP, "Cannot back up save slot %s", dst);
    }
    if (!RenameSaveDir(stagepath, targetpath))
    {
        if (existing && !RenameSaveDir(backup, targetpath))
        {
            Com_Error(ERR_DROP, "Cannot replace save; previous save is in %s", backup);
        }
        Com_Error(ERR_DROP, "Cannot replace save slot %s", dst);
    }
    if (existing)
    {
        char oldslot[80];
        Com_sprintf(oldslot, sizeof(oldslot), "%s.old", stage);
        SV_WipeSavegame(oldslot);
        /* Unexpected extra files are preserved in the backup directory. */
        _rmdir(backup);
    }
}

/*
==============
SV_WriteLevelFile

==============
*/
void SV_WriteLevelFile(void)
{
    char name[MAX_OSPATH];
    FILE *f;

    Com_DPrintf("SV_WriteLevelFile()\n");

    Com_sprintf(name, sizeof(name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.name);
    FS_CreatePath(name);
    f = fopen(name, "wb");
    if (!f)
    {
        Com_Error(ERR_DROP, "Cannot write %s", name);
    }
    SV_WriteSaveData(f, sv.configstrings, sizeof(sv.configstrings));
    CM_WritePortalState(f);
    SV_CloseSave(f);

    Com_sprintf(name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
    ge->WriteLevel(name);
}

/*
==============
SV_ReadLevelFile

==============
*/
void SV_ReadLevelFile(void)
{
    char name[MAX_OSPATH];
    FILE *f;

    Com_DPrintf("SV_ReadLevelFile()\n");

    Com_sprintf(name, sizeof(name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.name);
    f = fopen(name, "rb");
    if (!f)
    {
        Com_Error(ERR_DROP, "Cannot read %s", name);
    }
    FS_Read(sv.configstrings, sizeof(sv.configstrings), f);
    CM_ReadPortalState(f);
    fclose(f);

    Com_sprintf(name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
    ge->ReadLevel(name);
}

/*
==============
SV_WriteServerFile

==============
*/
void SV_WriteServerFile(qboolean autosave)
{
    FILE *f;
    cvar_t *var;
    char name[MAX_OSPATH], string[128];
    char comment[32];
    time_t aclock;
    struct tm *newtime;

    Com_DPrintf("SV_WriteServerFile(%s)\n", autosave ? "true" : "false");

    Com_sprintf(name, sizeof(name), "%s/save/current/server.ssv", FS_Gamedir());
    FS_CreatePath(name);
    f = fopen(name, "wb");
    if (!f)
    {
        Com_Error(ERR_DROP, "Cannot write %s", name);
    }
    // write the comment field
    memset(comment, 0, sizeof(comment));

    if (!autosave)
    {
        time(&aclock);
        newtime = localtime(&aclock);
        Com_sprintf(comment, sizeof(comment), "%2i:%i%i %2i/%2i  ", newtime->tm_hour,
                    newtime->tm_min / 10, newtime->tm_min % 10, newtime->tm_mon + 1,
                    newtime->tm_mday);
        strncat(comment, sv.configstrings[CS_NAME], sizeof(comment) - 1 - strlen(comment));
    }
    else
    { // autosaved
        Com_sprintf(comment, sizeof(comment), "ENTERING %s", sv.configstrings[CS_NAME]);
    }

    if (!autosave && Cmd_Argc() == 3 && !strcmp(Cmd_Argv(0), "save"))
    {
        Com_sprintf(comment, sizeof(comment), "%s", Cmd_Argv(2));
        for (char *c = comment; *c; c++)
        {
            if ((unsigned char)*c < 32 || (unsigned char)*c > 126)
            {
                *c = ' ';
            }
        }
    }
    SV_WriteSaveData(f, comment, sizeof(comment));

    // write the mapcmd
    SV_WriteSaveData(f, svs.mapcmd, sizeof(svs.mapcmd));

    // write all CVAR_LATCH cvars
    // these will be things like coop, skill, deathmatch, etc
    for (var = cvar_vars; var; var = var->next)
    {
        /* MENUES.cpp saves respawn rules with the world. These port cvars can
           change immediately, so CVAR_LATCH alone does not capture them. */
        if (!(var->flags & CVAR_LATCH) && strcmp(var->name, "respawn_monsters") &&
            strcmp(var->name, "respawn_items") && strcmp(var->name, "sv_gravity"))
        {
            continue;
        }
        if (strlen(var->name) >= sizeof(name) - 1 || strlen(var->string) >= sizeof(string) - 1)
        {
            Com_Printf("Cvar too long: %s = %s\n", var->name, var->string);
            continue;
        }
        memset(name, 0, sizeof(name));
        memset(string, 0, sizeof(string));
        strcpy(name, var->name);
        strcpy(string, var->string);
        SV_WriteSaveData(f, name, sizeof(name));
        SV_WriteSaveData(f, string, sizeof(string));
    }

    SV_CloseSave(f);

    // write game state
    Com_sprintf(name, sizeof(name), "%s/save/current/game.ssv", FS_Gamedir());
    ge->WriteGame(name, autosave);
}

/*
==============
SV_ReadServerFile

==============
*/
void SV_ReadServerFile(void)
{
    FILE *f;
    char name[MAX_OSPATH], string[128];
    char comment[32];
    char mapcmd[MAX_TOKEN_CHARS];

    Com_DPrintf("SV_ReadServerFile()\n");

    Com_sprintf(name, sizeof(name), "%s/save/current/server.ssv", FS_Gamedir());
    f = fopen(name, "rb");
    if (!f)
    {
        Com_Error(ERR_DROP, "Cannot read %s", name);
    }
    // read the comment field
    if (fread(comment, 1, sizeof(comment), f) != sizeof(comment) ||
        fread(mapcmd, 1, sizeof(mapcmd), f) != sizeof(mapcmd) ||
        !memchr(mapcmd, 0, sizeof(mapcmd)) || !mapcmd[0])
    {
        fclose(f);
        Com_Error(ERR_DROP, "Invalid server save header");
    }

    // read all CVAR_LATCH cvars
    // these will be things like coop, skill, deathmatch, etc
    while (1)
    {
        size_t count = fread(name, 1, sizeof(name), f);
        if (!count && !ferror(f))
        {
            break;
        }
        if (count != sizeof(name) || fread(string, 1, sizeof(string), f) != sizeof(string) ||
            !memchr(name, 0, sizeof(name)) || !name[0] || !memchr(string, 0, sizeof(string)))
        {
            fclose(f);
            Com_Error(ERR_DROP, "Invalid server save settings");
        }
        Com_DPrintf("Set %s = %s\n", name, string);
        Cvar_ForceSet(name, string);
    }

    fclose(f);

    // start a new game fresh with new cvars
    SV_InitGame();

    strcpy(svs.mapcmd, mapcmd);

    // read game state
    Com_sprintf(name, sizeof(name), "%s/save/current/game.ssv", FS_Gamedir());
    ge->ReadGame(name);
}

//=========================================================

/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f(void)
{
    SV_Map(true, Cmd_Argv(1), false);
}

/*
==================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
==================
*/
void SV_GameMap_f(void)
{
    char *map;
    int i;
    client_t *cl;
    qboolean *savedInuse;

    if (Cmd_Argc() != 2)
    {
        Com_Printf("USAGE: gamemap <map>\n");
        return;
    }

    Com_DPrintf("SV_GameMap(%s)\n", Cmd_Argv(1));

    FS_CreatePath(va("%s/save/current/", FS_Gamedir()));

    // check for clearing the current savegame
    map = Cmd_Argv(1);
    if (map[0] == '*')
    {
        // wipe all the *.sav files
        SV_WipeSavegame("current");
    }
    else
    { // save the map just exited
        if (sv.state == ss_game)
        {
            // clear all the client inuse flags before saving so that
            // when the level is re-entered, the clients will spawn
            // at spawn points instead of occupying body shells
            savedInuse = (qboolean *)(malloc(maxclients.GetFloat() * sizeof(qboolean)));
            for (i = 0, cl = svs.clients; i < maxclients.GetFloat(); i++, cl++)
            {
                savedInuse[i] = cl->edict->inuse;
                cl->edict->inuse = false;
            }

            SV_WriteLevelFile();

            // we must restore these for clients to transfer over correctly
            for (i = 0, cl = svs.clients; i < maxclients.GetFloat(); i++, cl++)
            {
                cl->edict->inuse = savedInuse[i];
            }
            free(savedInuse);
        }
    }

    // start up the next map
    SV_Map(false, Cmd_Argv(1), false);

    // archive server state
    strncpy(svs.mapcmd, Cmd_Argv(1), sizeof(svs.mapcmd) - 1);

    // copy off the level to the autosave slot
    if (!dedicated->value)
    {
        SV_WriteServerFile(true);
        SV_CopySaveGame("current", "save0");
    }
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
void SV_Map_f(void)
{
    char *map;
    char expanded[MAX_QPATH];

    // if not a pcx, demo, or cinematic, check to make sure the level exists
    map = Cmd_Argv(1);
    if (!strstr(map, "."))
    {
        Com_sprintf(expanded, sizeof(expanded), "maps/%s.map", map);
        if (FS_LoadFile(expanded, NULL) == -1)
        {
            Com_Printf("Can't find %s\n", expanded);
            return;
        }
    }

    sv.state = ss_dead; // don't save current level when changing
    SV_WipeSavegame("current");
    SV_GameMap_f();
}

/*
=====================================================================

  SAVEGAMES

=====================================================================
*/

static qboolean SV_ValidSaveDir(const char *dir)
{
    /* current is the engine's working directory; loading it would erase the
       source during SV_CopySaveGame. Restrict names before any file changes. */
    if (!dir[0] || strlen(dir) > 32 || !Q_strcasecmp(dir, "current"))
    {
        return false;
    }
    for (const unsigned char *c = (const unsigned char *)dir; *c; c++)
    {
        if (!isalnum(*c) && *c != '_' && *c != '-')
        {
            return false;
        }
    }
    return true;
}

/*
==============
SV_Loadgame_f

==============
*/
void SV_Loadgame_f(void)
{
    char name[MAX_OSPATH];
    FILE *f;
    char *dir;

    if (Cmd_Argc() != 2)
    {
        Com_Printf("USAGE: loadgame <directory>\n");
        return;
    }

    Com_Printf("Loading game...\n");

    dir = Cmd_Argv(1);
    if (!SV_ValidSaveDir(dir))
    {
        Com_Printf("Bad savedir.\n");
        return;
    }

    // make sure the server.ssv file exists
    Com_sprintf(name, sizeof(name), "%s/save/%s/server.ssv", FS_Gamedir(), Cmd_Argv(1));
    f = fopen(name, "rb");
    if (!f)
    {
        Com_Printf("No such savegame: %s\n", name);
        return;
    }
    fclose(f);

    SV_CopySaveGame(Cmd_Argv(1), "current");

    SV_ReadServerFile();

    // go to the map
    sv.state = ss_dead; // don't save current level when changing
    SV_Map(false, svs.mapcmd, true);
}

/*
==============
SV_Savegame_f

==============
*/
void SV_Savegame_f(void)
{
    char *dir;

    if (sv.state != ss_game)
    {
        Com_Printf("You must be in a game to save.\n");
        return;
    }

    if (Cmd_Argc() < 2 || Cmd_Argc() > 3)
    {
        Com_Printf("USAGE: save <directory> [\"description\"]\n");
        return;
    }

    if (Cvar_VariableValue("deathmatch"))
    {
        Com_Printf("Can't savegame in a deathmatch\n");
        return;
    }

    if (maxclients.GetFloat() == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)
    {
        Com_Printf("\nCan't savegame while dead!\n");
        return;
    }

    dir = Cmd_Argv(1);
    if (!SV_ValidSaveDir(dir))
    {
        Com_Printf("Bad savedir.\n");
        return;
    }

    Com_Printf("Saving game...\n");

    // archive current level, including all client edicts.
    // when the level is reloaded, they will be shells awaiting
    // a connecting client
    SV_WriteLevelFile();

    // save server state
    SV_WriteServerFile(false);

    // copy it off
    SV_CopySaveGame("current", dir);

    Com_Printf("Done.\n");
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f(void)
{
    if (!svs.initialized)
    {
        Com_Printf("No server running.\n");
        return;
    }

    if (Cmd_Argc() != 2)
    {
        Com_Printf("Usage: kick <userid>\n");
        return;
    }

    if (!SV_SetPlayer())
    {
        return;
    }

    SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", sv_client->name);
    // print directly, because the dropped client won't get the
    // SV_BroadcastPrintf message
    SV_ClientPrintf(sv_client, PRINT_HIGH, "You were kicked from the game\n");
    SV_DropClient(sv_client);
    sv_client->lastmessage = svs.realtime; // min case there is a funny zombie
}

/*
================
SV_Status_f
================
*/
void SV_Status_f(void)
{
    int i, j, l;
    client_t *cl;
    char *s;
    int ping;
    if (!svs.clients)
    {
        Com_Printf("No server running.\n");
        return;
    }
    Com_Printf("map              : %s\n", sv.name);

    Com_Printf("num score ping name            lastmsg address               qport \n");
    Com_Printf("--- ----- ---- --------------- ------- --------------------- ------\n");
    for (i = 0, cl = svs.clients; i < maxclients.GetFloat(); i++, cl++)
    {
        if (!cl->state)
        {
            continue;
        }
        Com_Printf("%3i ", i);
        Com_Printf("%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);

        if (cl->state == cs_connected)
        {
            Com_Printf("CNCT ");
        }
        else if (cl->state == cs_zombie)
        {
            Com_Printf("ZMBI ");
        }
        else
        {
            ping = cl->ping < 9999 ? cl->ping : 9999;
            Com_Printf("%4i ", ping);
        }

        Com_Printf("%s", cl->name);
        l = 16 - strlen(cl->name);
        for (j = 0; j < l; j++)
        {
            Com_Printf(" ");
        }

        Com_Printf("%7i ", svs.realtime - cl->lastmessage);

        s = NET_AdrToString(cl->netchan.remote_address);
        Com_Printf("%s", s);
        l = 22 - strlen(s);
        for (j = 0; j < l; j++)
        {
            Com_Printf(" ");
        }

        Com_Printf("%5i", cl->netchan.qport);

        Com_Printf("\n");
    }
    Com_Printf("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
    client_t *client;
    int j;
    char *p;
    char text[1024];

    if (Cmd_Argc() < 2)
    {
        return;
    }

    strcpy(text, "console: ");
    p = Cmd_Args();

    if (*p == '"')
    {
        p++;
        p[strlen(p) - 1] = 0;
    }

    strcat(text, p);

    for (j = 0, client = svs.clients; j < maxclients.GetFloat(); j++, client++)
    {
        if (client->state != cs_spawned)
        {
            continue;
        }
        SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
    }
}

/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f(void)
{
    svs.last_heartbeat = -9999999;
}

/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f(void)
{
    Com_Printf("Server info settings:\n");
    Info_Print(Cvar_Serverinfo());
}

/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
void SV_DumpUser_f(void)
{
    if (Cmd_Argc() != 2)
    {
        Com_Printf("Usage: info <userid>\n");
        return;
    }

    if (!SV_SetPlayer())
    {
        return;
    }

    Com_Printf("userinfo\n");
    Com_Printf("--------\n");
    Info_Print(sv_client->userinfo);
}

/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
void SV_ServerRecord_f(void)
{
    char name[MAX_OSPATH];
    char buf_data[32768];
    sizebuf_t buf;
    int len;
    int i;

    if (Cmd_Argc() != 2)
    {
        Com_Printf("serverrecord <demoname>\n");
        return;
    }

    if (svs.demofile)
    {
        Com_Printf("Already recording.\n");
        return;
    }

    if (sv.state != ss_game)
    {
        Com_Printf("You must be in a level to record.\n");
        return;
    }

    //
    // open the demo file
    //
    Com_sprintf(name, sizeof(name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

    Com_Printf("recording to %s.\n", name);
    FS_CreatePath(name);
    svs.demofile = fopen(name, "wb");
    if (!svs.demofile)
    {
        Com_Printf("ERROR: couldn't open.\n");
        return;
    }

    // setup a buffer to catch all multicasts
    SZ_Init(&svs.demo_multicast, svs.demo_multicast_buf, sizeof(svs.demo_multicast_buf));

    //
    // write a single giant fake message with all the startup info
    //
    SZ_Init(&buf, buf_data, sizeof(buf_data));

    //
    // serverdata needs to go over for all types of servers
    // to make sure the protocol is right, and to set the gamedir
    //
    // send the serverdata
    MSG_WriteByte(&buf, svc_serverdata);
    MSG_WriteLong(&buf, PROTOCOL_VERSION);
    MSG_WriteLong(&buf, svs.spawncount);
    // 2 means server demo
    MSG_WriteByte(&buf, 2); // demos are always attract loops
    MSG_WriteString(&buf, Cvar_VariableString("gamedir"));
    MSG_WriteShort(&buf, -1);
    // send full levelname
    MSG_WriteString(&buf, sv.configstrings[CS_NAME]);

    for (i = 0; i < MAX_CONFIGSTRINGS; i++)
    {
        if (sv.configstrings[i][0])
        {
            MSG_WriteByte(&buf, svc_configstring);
            MSG_WriteShort(&buf, i);
            MSG_WriteString(&buf, sv.configstrings[i]);
        }
    }

    // write it to the demo file
    Com_DPrintf("signon message length: %i\n", buf.cursize);
    len = LittleLong(buf.cursize);
    fwrite(&len, 4, 1, svs.demofile);
    fwrite(buf.data, buf.cursize, 1, svs.demofile);

    // the rest of the demo file will be individual frames
}

/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
void SV_ServerStop_f(void)
{
    if (!svs.demofile)
    {
        Com_Printf("Not doing a serverrecord.\n");
        return;
    }
    fclose(svs.demofile);
    svs.demofile = NULL;
    Com_Printf("Recording completed.\n");
}

/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game

===============
*/
void SV_KillServer_f(void)
{
    if (!svs.initialized)
    {
        return;
    }
    SV_Shutdown("Server was killed.\n", false);
    NET_Config(false); // close network sockets
}

/*
===============
SV_ServerCommand_f

Let the game dll handle a command
===============
*/
void SV_ServerCommand_f(void)
{
    if (!ge)
    {
        Com_Printf("No game loaded.\n");
        return;
    }

    ge->ServerCommand();
}

//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands(void)
{
    Cmd_AddCommand("heartbeat", SV_Heartbeat_f);
    Cmd_AddCommand("kick", SV_Kick_f);
    Cmd_AddCommand("status", SV_Status_f);
    Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
    Cmd_AddCommand("dumpuser", SV_DumpUser_f);

    Cmd_AddCommand("map", SV_Map_f);
    Cmd_AddCommand("demomap", SV_DemoMap_f);
    Cmd_AddCommand("gamemap", SV_GameMap_f);
    Cmd_AddCommand("setmaster", SV_SetMaster_f);

    if (dedicated->value)
    {
        Cmd_AddCommand("say", SV_ConSay_f);
    }

    Cmd_AddCommand("serverrecord", SV_ServerRecord_f);
    Cmd_AddCommand("serverstop", SV_ServerStop_f);

    Cmd_AddCommand("save", SV_Savegame_f);
    Cmd_AddCommand("load", SV_Loadgame_f);

    Cmd_AddCommand("killserver", SV_KillServer_f);

    Cmd_AddCommand("sv", SV_ServerCommand_f);
}
