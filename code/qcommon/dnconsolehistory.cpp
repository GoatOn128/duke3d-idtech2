/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// Port of the Doom 3 BFG Edition framework ConsoleHistory.cpp. BFG
// reads and writes the history through idFile; this port uses the Quake
// 2 filesystem gamedir with stdio, the same pattern as the condump
// command.

#include "qcommon.h"
#include "dnconsolehistory.h"

DN_ConsoleHistory consoleHistory;

#define HISTORY_FILE_NAME "consoleHistory.txt"

DN_ConsoleHistory::DN_ConsoleHistory(void)
{
    ClearHistory();
}

/*
========================
DN_ConsoleHistory::AddToHistory
========================
*/
void DN_ConsoleHistory::AddToHistory(const char *line, qboolean writeHistoryFile)
{
    FILE *f;
    char name[MAX_OSPATH];
    const char *s;
    int i;

    // empty lines never modify history
    if (line == NULL)
        return;
    for (s = line; *s != 0; s++)
    {
        if (*s > ' ')
            break;
    }
    if (*s == 0)
        return;

    // repeating the last command doesn't add to the list
    if (Q_strcasecmp(historyLines[(numHistory - 1) & (COMMAND_HISTORY - 1)], line) == 0)
    {
        if (Q_strcasecmp(historyLines[returnLine & (COMMAND_HISTORY - 1)], line) == 0)
        {
            // the command was retrieved from the history, so move the
            // up point down so that up arrow will retrieve the same
            // command again
            upPoint = returnLine;
        }
        else
        {
            // the command was typed again, so leave the up/down points alone
        }
        return;
    }

    // if we had previously retrieved older history commands with up
    // arrow, the up/down points are reset to the end where this
    // command is added
    upPoint = numHistory;
    returnLine = numHistory;
    downPoint = numHistory + 1;

    strncpy(historyLines[numHistory & (COMMAND_HISTORY - 1)], line, DN_MAX_EDIT_LINE - 1);
    historyLines[numHistory & (COMMAND_HISTORY - 1)][DN_MAX_EDIT_LINE - 1] = 0;
    numHistory++;

    // write the history file to disk
    if (writeHistoryFile)
    {
        Com_sprintf(name, sizeof(name), "%s/" HISTORY_FILE_NAME, FS_Gamedir());
        FS_CreatePath(name);
        f = fopen(name, "w");
        if (f != NULL)
        {
            for (i = numHistory - COMMAND_HISTORY; i < numHistory; i++)
            {
                if (i < 0)
                    continue;
                fprintf(f, "%s\n", historyLines[i & (COMMAND_HISTORY - 1)]);
            }
            fclose(f);
        }
    }
}

/*
========================
DN_ConsoleHistory::RetrieveFromHistory
========================
*/
const char *DN_ConsoleHistory::RetrieveFromHistory(qboolean backward)
{
    // if there are no commands in the history
    if (numHistory == 0)
        return "";

    // move the history point
    if (backward)
    {
        if (upPoint < numHistory - COMMAND_HISTORY || upPoint < 0)
            return "";
        returnLine = upPoint;
        downPoint = upPoint + 1;
        upPoint--;
    }
    else
    {
        if (downPoint >= numHistory)
        {
            upPoint = downPoint - 1; // so pressing UP after walking past the end returns the last command
            return "";
        }
        returnLine = downPoint;
        upPoint = downPoint - 1;
        downPoint++;
    }
    return historyLines[returnLine & (COMMAND_HISTORY - 1)];
}

/*
========================
DN_ConsoleHistory::LoadHistoryFile
========================
*/
void DN_ConsoleHistory::LoadHistoryFile(void)
{
    FILE *f;
    char name[MAX_OSPATH];
    char line[DN_MAX_EDIT_LINE];
    int i;

    Com_sprintf(name, sizeof(name), "%s/" HISTORY_FILE_NAME, FS_Gamedir());
    f = fopen(name, "r");
    if (f == NULL)
        return;

    while (fgets(line, sizeof(line), f))
    {
        // strip trailing whitespace, including the newline
        i = (int)strlen(line) - 1;
        for (; i >= 0; i--)
        {
            if (line[i] <= ' ')
                line[i] = 0;
            else
                break;
        }
        if (line[0] == 0)
            break;
        AddToHistory(line, false);
    }

    fclose(f);
}

/*
========================
DN_ConsoleHistory::PrintHistory
========================
*/
void DN_ConsoleHistory::PrintHistory(void)
{
    int i;

    for (i = numHistory - COMMAND_HISTORY; i < numHistory; i++)
    {
        if (i < 0)
            continue;
        Com_Printf("%c%c%c%4i: %s\n", i == upPoint ? 'U' : ' ', i == downPoint ? 'D' : ' ',
                   i == returnLine ? 'R' : ' ', i, historyLines[i & (COMMAND_HISTORY - 1)]);
    }
}

/*
========================
DN_ConsoleHistory::ClearHistory
========================
*/
void DN_ConsoleHistory::ClearHistory(void)
{
    upPoint = 0;
    downPoint = 0;
    returnLine = 0;
    numHistory = 0;
    memset(historyLines, 0, sizeof(historyLines));
}

/*
========================
history and clearhistory
========================
*/
static void Con_History_f(void)
{
    consoleHistory.PrintHistory();
}

static void Con_ClearHistory_f(void)
{
    consoleHistory.ClearHistory();
}

void DN_ConsoleHistoryInit(void)
{
    Cmd_AddCommand("history", Con_History_f);
    Cmd_AddCommand("clearhistory", Con_ClearHistory_f);
    consoleHistory.LoadHistoryFile();
}
