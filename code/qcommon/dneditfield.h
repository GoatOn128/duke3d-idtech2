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

// Port of the Doom 3 BFG Edition framework edit field (EditField.h/.cpp)
// to the Quake 2 common layer. BFG draws the field through renderSystem
// calls; this port draws through a caller supplied character callback so
// the module stays out of the client.

#ifndef DNEDITFIELD_H
#define DNEDITFIELD_H

#define DN_MAX_EDIT_LINE 256

typedef struct dn_autoComplete_s
{
    qboolean valid;
    int length;
    char completionString[DN_MAX_EDIT_LINE];
    char currentMatch[DN_MAX_EDIT_LINE];
    int matchCount;
    int matchIndex;
    int findMatchIndex;
} dn_autoComplete_t;

class DN_EditField
{
public:
    DN_EditField(void);

    void Clear(void);
    void SetWidthInChars(int w);
    void SetCursor(int c);
    int GetCursor(void) const;
    void ClearAutoComplete(void);
    int GetAutoCompleteLength(void) const;
    void AutoComplete(void);
    void CharEvent(int c);
    // Q2 has no key and char event split, so the BFG idKeyInput
    // modifier polling became parameters on this entry point.
    void KeyDownEvent(int key, qboolean ctrlDown, qboolean shiftDown);
    void Paste(void);
    char *GetBuffer(void);
    void SetBuffer(const char *buffer);
    // Draw through the caller supplied callback, one 8 pixel console
    // character at a time. showCursor is the blink phase.
    void Draw(int x, int y, qboolean showCursor, void (*drawChar)(int x, int y, int c));

private:
    int cursor;
    int scroll;
    int widthInChars;
    char buffer[DN_MAX_EDIT_LINE];
    dn_autoComplete_t autoComplete;
};

// BFG keeps this state in idKeyInput; the Quake 2 client has no
// overstrike concept of its own, so it lives with the edit field.
qboolean DN_GetOverstrikeMode(void);
void DN_SetOverstrikeMode(qboolean overstrike);

#endif
