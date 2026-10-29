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

// Port of the Doom 3 BFG Edition framework EditField.cpp. The BFG
// command argument completion callbacks are dropped: the Quake 2 cmd
// and cvar systems have no argument completion infrastructure, so only
// commands, aliases and cvars complete. Everything else follows the BFG
// source.

#include "qcommon.h"
#include "dneditfield.h"
#include "../client/keys.h"

static dn_autoComplete_t globalAutoComplete;
static qboolean overstrikeMode = false;

// bounded copy that always terminates, like BFG idStr::Copynz
static void Copynz(char *dest, const char *src, int size)
{
    if (size <= 0)
        return;
    strncpy(dest, src, size - 1);
    dest[size - 1] = 0;
}

// Split the buffer the way BFG idCmdArgs does for completion: the first
// token becomes the completion string, the rest of the line the args.
static void TokenizeForCompletion(const char *text, char *command, int commandSize, char *args,
                                  int argsSize)
{
    int i;

    while (*text && *text <= ' ')
        text++;

    for (i = 0; text[i] && text[i] > ' ' && i < commandSize - 1; i++)
        command[i] = text[i];
    command[i] = 0;

    while (*text && *text > ' ')
        text++;
    while (*text && *text <= ' ')
        text++;

    Copynz(args, text, argsSize);
}

/*
===============
FindMatches
===============
*/
static void FindMatches(const char *s)
{
    int i;

    if (Q_strncasecmp(s, globalAutoComplete.completionString,
                      (int)strlen(globalAutoComplete.completionString)) != 0)
        return;
    globalAutoComplete.matchCount++;
    if (globalAutoComplete.matchCount == 1)
    {
        Copynz(globalAutoComplete.currentMatch, s, sizeof(globalAutoComplete.currentMatch));
        return;
    }

    // cut currentMatch to the amount common with s
    for (i = 0; s[i]; i++)
    {
        if (tolower(globalAutoComplete.currentMatch[i]) != tolower(s[i]))
        {
            globalAutoComplete.currentMatch[i] = 0;
            break;
        }
    }
    globalAutoComplete.currentMatch[i] = 0;
}

/*
===============
FindIndexMatch
===============
*/
static void FindIndexMatch(const char *s)
{
    if (Q_strncasecmp(s, globalAutoComplete.completionString,
                      (int)strlen(globalAutoComplete.completionString)) != 0)
        return;

    if (globalAutoComplete.findMatchIndex == globalAutoComplete.matchIndex)
        Copynz(globalAutoComplete.currentMatch, s, sizeof(globalAutoComplete.currentMatch));

    globalAutoComplete.findMatchIndex++;
}

/*
===============
PrintMatches
===============
*/
static void PrintMatches(const char *s)
{
    if (Q_strncasecmp(s, globalAutoComplete.currentMatch,
                      (int)strlen(globalAutoComplete.currentMatch)) == 0)
        Com_Printf("    %s\n", s);
}

/*
===============
PrintCvarMatches
===============
*/
static void PrintCvarMatches(const char *s)
{
    if (Q_strncasecmp(s, globalAutoComplete.currentMatch,
                      (int)strlen(globalAutoComplete.currentMatch)) == 0)
        Com_Printf("    %s = \"%s\"\n", s, Cvar_VariableString((char *)s));
}

qboolean DN_GetOverstrikeMode(void)
{
    return overstrikeMode;
}

void DN_SetOverstrikeMode(qboolean overstrike)
{
    overstrikeMode = overstrike;
}

/*
===============
DN_EditField::DN_EditField
===============
*/
DN_EditField::DN_EditField(void)
{
    widthInChars = 0;
    Clear();
}

/*
===============
DN_EditField::Clear
===============
*/
void DN_EditField::Clear(void)
{
    buffer[0] = 0;
    cursor = 0;
    scroll = 0;
    autoComplete.length = 0;
    autoComplete.valid = false;
}

/*
===============
DN_EditField::SetWidthInChars
===============
*/
void DN_EditField::SetWidthInChars(int w)
{
    assert(w <= DN_MAX_EDIT_LINE);
    widthInChars = w;
}

/*
===============
DN_EditField::SetCursor
===============
*/
void DN_EditField::SetCursor(int c)
{
    assert(c <= DN_MAX_EDIT_LINE);
    cursor = c;
}

/*
===============
DN_EditField::GetCursor
===============
*/
int DN_EditField::GetCursor(void) const
{
    return cursor;
}

/*
===============
DN_EditField::ClearAutoComplete
===============
*/
void DN_EditField::ClearAutoComplete(void)
{
    if (autoComplete.length > 0 && autoComplete.length <= (int)strlen(buffer))
    {
        buffer[autoComplete.length] = 0;
        if (cursor > autoComplete.length)
            cursor = autoComplete.length;
    }
    autoComplete.length = 0;
    autoComplete.valid = false;
}

/*
===============
DN_EditField::GetAutoCompleteLength
===============
*/
int DN_EditField::GetAutoCompleteLength(void) const
{
    return autoComplete.length;
}

/*
===============
DN_EditField::AutoComplete
===============
*/
void DN_EditField::AutoComplete(void)
{
    char completionArgString[DN_MAX_EDIT_LINE];

    if (!autoComplete.valid)
    {
        TokenizeForCompletion(buffer, autoComplete.completionString,
                              sizeof(autoComplete.completionString), completionArgString,
                              sizeof(completionArgString));
        autoComplete.matchCount = 0;
        autoComplete.matchIndex = 0;
        autoComplete.currentMatch[0] = 0;

        if (strlen(autoComplete.completionString) == 0)
            return;

        globalAutoComplete = autoComplete;

        Cmd_CommandCompletion(FindMatches);
        Cvar_CommandCompletion(FindMatches);

        autoComplete = globalAutoComplete;

        if (autoComplete.matchCount == 0)
            return; // no matches

        // complete to the shortest common match, keep any arguments
        Copynz(buffer, autoComplete.currentMatch, sizeof(buffer));
        if (completionArgString[0])
        {
            strncat(buffer, " ", sizeof(buffer) - strlen(buffer) - 1);
            strncat(buffer, completionArgString, sizeof(buffer) - strlen(buffer) - 1);
        }

        autoComplete.length = (int)strlen(buffer);
        autoComplete.valid = (autoComplete.matchCount != 1);
        SetCursor(autoComplete.length);

        Com_Printf("]%s\n", buffer);

        // run through again, printing matches
        globalAutoComplete = autoComplete;

        Cmd_CommandCompletion(PrintMatches);
        Cvar_CommandCompletion(PrintCvarMatches);
    }
    else if (autoComplete.matchCount != 1)
    {
        // get the next match and show instead
        autoComplete.matchIndex++;
        if (autoComplete.matchIndex == autoComplete.matchCount)
            autoComplete.matchIndex = 0;
        autoComplete.findMatchIndex = 0;

        globalAutoComplete = autoComplete;

        Cmd_CommandCompletion(FindIndexMatch);
        Cvar_CommandCompletion(FindIndexMatch);

        autoComplete = globalAutoComplete;

        // and print it
        Copynz(buffer, autoComplete.currentMatch, sizeof(buffer));
        if (autoComplete.length > (int)strlen(buffer))
            autoComplete.length = (int)strlen(buffer);
        SetCursor(autoComplete.length);
    }
}

/*
===============
DN_EditField::CharEvent
===============
*/
void DN_EditField::CharEvent(int ch)
{
    int len;

    if (ch == 'v' - 'a' + 1)
    { // ctrl-v is paste
        Paste();
        return;
    }

    if (ch == 'c' - 'a' + 1)
    { // ctrl-c clears the field
        Clear();
        return;
    }

    len = (int)strlen(buffer);

    if (ch == 'h' - 'a' + 1 || ch == K_BACKSPACE)
    { // ctrl-h is backspace
        if (cursor > 0)
        {
            memmove(buffer + cursor - 1, buffer + cursor, len + 1 - cursor);
            cursor--;
            if (cursor < scroll)
                scroll--;
        }
        return;
    }

    if (ch == 'a' - 'a' + 1)
    { // ctrl-a is home
        cursor = 0;
        scroll = 0;
        return;
    }

    if (ch == 'e' - 'a' + 1)
    { // ctrl-e is end
        cursor = len;
        scroll = cursor - widthInChars;
        return;
    }

    // ignore any other non printable chars
    if (ch < 32)
        return;

    if (DN_GetOverstrikeMode())
    {
        if (cursor == DN_MAX_EDIT_LINE - 1)
            return;
        buffer[cursor] = ch;
        cursor++;
    }
    else
    { // insert mode
        if (len == DN_MAX_EDIT_LINE - 1)
            return; // all full
        memmove(buffer + cursor + 1, buffer + cursor, len + 1 - cursor);
        buffer[cursor] = ch;
        cursor++;
    }

    if (cursor >= widthInChars)
        scroll++;

    if (cursor == len + 1)
        buffer[cursor] = 0;
}

/*
===============
DN_EditField::KeyDownEvent
===============
*/
void DN_EditField::KeyDownEvent(int key, qboolean ctrlDown, qboolean shiftDown)
{
    int len;

    // shift-insert is paste
    if ((key == K_INS || key == K_KP_INS) && shiftDown)
    {
        ClearAutoComplete();
        Paste();
        return;
    }

    len = (int)strlen(buffer);

    if (key == K_DEL)
    {
        if (autoComplete.length)
            ClearAutoComplete();
        else if (cursor < len)
            memmove(buffer + cursor, buffer + cursor + 1, len - cursor);
        return;
    }

    if (key == K_RIGHTARROW)
    {
        if (ctrlDown)
        {
            // skip to next word
            while ((cursor < len) && (buffer[cursor] != ' '))
                cursor++;

            while ((cursor < len) && (buffer[cursor] == ' '))
                cursor++;
        }
        else
            cursor++;

        if (cursor > len)
            cursor = len;

        if (cursor >= scroll + widthInChars)
            scroll = cursor - widthInChars + 1;

        if (autoComplete.length > 0)
            autoComplete.length = cursor;
        return;
    }

    if (key == K_LEFTARROW)
    {
        if (ctrlDown)
        {
            // skip to previous word
            while ((cursor > 0) && (buffer[cursor - 1] == ' '))
                cursor--;

            while ((cursor > 0) && (buffer[cursor - 1] != ' '))
                cursor--;
        }
        else
            cursor--;

        if (cursor < 0)
            cursor = 0;
        if (cursor < scroll)
            scroll = cursor;

        if (autoComplete.length)
            autoComplete.length = cursor;
        return;
    }

    if (key == K_HOME || (key == 'a' && ctrlDown))
    {
        cursor = 0;
        scroll = 0;
        if (autoComplete.length)
        {
            autoComplete.length = cursor;
            autoComplete.valid = false;
        }
        return;
    }

    if (key == K_END || (key == 'e' && ctrlDown))
    {
        cursor = len;
        if (cursor >= scroll + widthInChars)
            scroll = cursor - widthInChars + 1;
        if (autoComplete.length)
        {
            autoComplete.length = cursor;
            autoComplete.valid = false;
        }
        return;
    }

    if (key == K_INS)
    {
        DN_SetOverstrikeMode(!DN_GetOverstrikeMode());
        return;
    }

    // clear autocompletion buffer on normal key input
    if (key != K_ALT && key != K_CTRL && key != K_SHIFT)
        ClearAutoComplete();
}

/*
===============
DN_EditField::Paste
===============
*/
void DN_EditField::Paste(void)
{
    char *cbd;
    int pasteLen, i;

    cbd = Sys_GetClipboardData();

    if (!cbd)
        return;

    // send as if typed, so insert / overstrike works properly
    pasteLen = (int)strlen(cbd);
    for (i = 0; i < pasteLen; i++)
        CharEvent(cbd[i]);

    free(cbd);
}

/*
===============
DN_EditField::GetBuffer
===============
*/
char *DN_EditField::GetBuffer(void)
{
    return buffer;
}

/*
===============
DN_EditField::SetBuffer
===============
*/
void DN_EditField::SetBuffer(const char *buf)
{
    Clear();
    Copynz(buffer, buf, sizeof(buffer));
    SetCursor((int)strlen(buffer));
}

/*
===============
DN_EditField::Draw

Port of the BFG edit field drawing on the Q2 8 pixel console grid.
===============
*/
void DN_EditField::Draw(int x, int y, qboolean showCursor, void (*drawChar)(int x, int y, int c))
{
    char str[DN_MAX_EDIT_LINE];
    int len, drawLen, prestep, cursorChar, i;

    drawLen = widthInChars;
    len = (int)strlen(buffer) + 1;

    // guarantee that the cursor will be visible
    if (len <= drawLen)
    {
        prestep = 0;
    }
    else
    {
        if (scroll + drawLen > len)
        {
            scroll = len - drawLen;
            if (scroll < 0)
                scroll = 0;
        }
        prestep = scroll;
    }

    if (prestep + drawLen > len)
        drawLen = len - prestep;

    if (drawLen >= DN_MAX_EDIT_LINE)
        return;

    memcpy(str, buffer + prestep, drawLen);
    str[drawLen] = 0;

    for (i = 0; str[i]; i++)
        drawChar(x + i * 8, y, str[i]);

    if (!showCursor)
        return;

    if (DN_GetOverstrikeMode())
        cursorChar = 11;
    else
        cursorChar = 10;

    drawChar(x + (cursor - prestep) * 8, y, cursorChar);
}
