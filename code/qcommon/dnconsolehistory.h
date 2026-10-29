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

// Port of the Doom 3 BFG Edition framework console history
// (ConsoleHistory.h/.cpp). Behaves like the windows command prompt,
// with the addition of a persistent history file. The BFG idStr lines
// became fixed char arrays.

#ifndef DNCONSOLEHISTORY_H
#define DNCONSOLEHISTORY_H

#include "dneditfield.h"

class DN_ConsoleHistory
{
public:
    DN_ConsoleHistory(void);

    void LoadHistoryFile(void);

    // the line should not have a newline. Empty lines are never added to
    // the history. If the command is the same as the last returned
    // history line, nothing is changed.
    void AddToHistory(const char *line, qboolean writeHistoryFile = true);

    // the string will not have a newline. Returns an empty string if
    // there is nothing to retrieve in that direction.
    const char *RetrieveFromHistory(qboolean backward);

    // console commands
    void PrintHistory(void);
    void ClearHistory(void);

private:
    enum
    {
        COMMAND_HISTORY = 64
        // COMMAND_HISTORY is a power of two: the ring wraps with a binary and
    };
    int upPoint;    // command to be retrieved with next up-arrow
    int downPoint;  // command to be retrieved with next down-arrow
    int returnLine; // last returned by RetrieveFromHistory()
    int numHistory;
    char historyLines[COMMAND_HISTORY][DN_MAX_EDIT_LINE];
};

extern DN_ConsoleHistory consoleHistory;

// registers the history and clearhistory commands and loads the
// persisted history file
void DN_ConsoleHistoryInit(void);

#endif
