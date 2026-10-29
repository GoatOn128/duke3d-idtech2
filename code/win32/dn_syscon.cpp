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

// Port of the Doom 3 BFG Edition win32 system console
// (sys/win32/win_syscon.cpp) to the Quake 2 platform layer. The BFG
// idEditField became DN_EditField; the Sys_QueEvent SE_CONSOLE quit path
// appends to the console input text drained by Qcommon_Frame, because Q2
// has no event queue at the system layer; and the BFG idStr color code
// stripping in Conbuf_AppendText became a high bit mask, since Q2 console
// text carries color in the character high bit and the win32 edit control
// cannot show it. QHOST (win32/conproc.cpp) is dormant with the system
// console in use.

#include <windows.h>

#include "../qcommon/qcommon.h"
#include "../qcommon/dneditfield.h"

#include "resource.h"
#include "dn_syscon.h"

#define COPY_ID     1
#define QUIT_ID     2
#define CLEAR_ID    3

#define ERRORBOX_ID 10
#define ERRORTEXT_ID 11

#define EDIT_ID     100
#define INPUT_ID    101

#define COMMAND_HISTORY 64

#define CONSOLE_WINDOW_CLASS "DN_WIN32_CONSOLE_CLASS"
#define CONSOLE_WINDOW_TITLE "duke3d"

// the BFG console reads DIK style scan codes from the message lParam:
// bits 16-23 hold the scan code, bit 24 the extended key flag which moves
// into bit 7. These are the scan codes the console window reacts to.
#define SC_ENTER     28      // BFG K_ENTER
#define SC_KP_8      72      // BFG K_KP_8
#define SC_KP_2      80      // BFG K_KP_2
#define SC_UPARROW   0xC8    // scan 0x48 plus the extended bit
#define SC_DOWNARROW 0xD0

typedef struct
{
    HWND hWnd;
    HWND hwndBuffer;

    HWND hwndButtonClear;
    HWND hwndButtonCopy;
    HWND hwndButtonQuit;

    HWND hwndErrorBox;

    HBRUSH hbrEditBackground;
    HBRUSH hbrErrorBackground;

    HFONT hfBufferFont;

    HWND hwndInputLine;

    char errorString[80];

    char consoleText[512], returnedText[512];
    qboolean quitOnClose;
    int windowWidth, windowHeight;

    WNDPROC SysInputLineWndProc;

    HINSTANCE hInstance;

    // the BFG command history ring, one edit field per line
    DN_EditField historyEditLines[COMMAND_HISTORY];

    int nextHistoryLine; // the last line in the history buffer, not masked
    int historyLine;     // the line being displayed from history buffer

    DN_EditField consoleField;
} WinConData;

static WinConData s_wcd;

/*
================
ConWndProc
================
*/
static LRESULT WINAPI ConWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static qboolean s_timePolarity;

    switch (uMsg)
    {
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE)
            {
                SetFocus(s_wcd.hwndInputLine);
            }
            break;
        case WM_CLOSE:
            if (s_wcd.quitOnClose)
            {
                PostQuitMessage(0);
            }
            else
            {
                Sys_ShowConsole(0, false);
                Cvar_SetValue("win_viewlog", 0);
            }
            return 0;
        case WM_CTLCOLORSTATIC:
            if ((HWND)lParam == s_wcd.hwndBuffer)
            {
                SetBkColor((HDC)wParam, RGB(0x00, 0x00, 0x80));
                SetTextColor((HDC)wParam, RGB(0xff, 0xff, 0x00));
                return (LRESULT)s_wcd.hbrEditBackground;
            }
            else if ((HWND)lParam == s_wcd.hwndErrorBox)
            {
                if (s_timePolarity & 1)
                {
                    SetBkColor((HDC)wParam, RGB(0x80, 0x80, 0x80));
                    SetTextColor((HDC)wParam, RGB(0xff, 0x0, 0x00));
                }
                else
                {
                    SetBkColor((HDC)wParam, RGB(0x80, 0x80, 0x80));
                    SetTextColor((HDC)wParam, RGB(0x00, 0x0, 0x00));
                }
                return (LRESULT)s_wcd.hbrErrorBackground;
            }
            break;
        case WM_SYSCOMMAND:
            if (wParam == SC_CLOSE)
            {
                PostQuitMessage(0);
            }
            break;
        case WM_COMMAND:
            if (wParam == COPY_ID)
            {
                SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
                SendMessage(s_wcd.hwndBuffer, WM_COPY, 0, 0);
            }
            else if (wParam == QUIT_ID)
            {
                if (s_wcd.quitOnClose)
                {
                    PostQuitMessage(0);
                }
                else
                {
                    // Q2 has no system event queue, so the quit button
                    // appends to the text drained by Qcommon_Frame's
                    // Sys_ConsoleInput loop
                    strncat(s_wcd.consoleText, "quit",
                            sizeof(s_wcd.consoleText) - strlen(s_wcd.consoleText) - 1);
                }
            }
            else if (wParam == CLEAR_ID)
            {
                SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
                SendMessage(s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, (LPARAM)"");
                UpdateWindow(s_wcd.hwndBuffer);
            }
            break;
        case WM_CREATE:
            s_wcd.hbrEditBackground = CreateSolidBrush(RGB(0x00, 0x00, 0x80));
            s_wcd.hbrErrorBackground = CreateSolidBrush(RGB(0x80, 0x80, 0x80));
            SetTimer(hWnd, 1, 1000, NULL);
            break;
        case WM_TIMER:
            if (wParam == 1)
            {
                // flip the error text color every second, like the BFG box
                s_timePolarity = !s_timePolarity;
                if (s_wcd.hwndErrorBox)
                {
                    InvalidateRect(s_wcd.hwndErrorBox, NULL, FALSE);
                }
            }
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
================
InputLineWndProc

The edit control does the text editing; the DN_EditField tracks the
buffer and cursor for history and command completion.
================
*/
static LRESULT WINAPI InputLineWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int key, cursor;

    switch (uMsg)
    {
        case WM_KILLFOCUS:
            if ((HWND)wParam == s_wcd.hWnd || (HWND)wParam == s_wcd.hwndErrorBox)
            {
                SetFocus(hWnd);
                return 0;
            }
            break;

        case WM_KEYDOWN:
            key = ((lParam >> 16) & 0xFF) | (((lParam >> 24) & 1) << 7);

            // command history
            if (key == SC_UPARROW || key == SC_KP_8)
            {
                if (s_wcd.nextHistoryLine - s_wcd.historyLine < COMMAND_HISTORY && s_wcd.historyLine > 0)
                {
                    s_wcd.historyLine--;
                }
                s_wcd.consoleField = s_wcd.historyEditLines[s_wcd.historyLine % COMMAND_HISTORY];

                SetWindowText(s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer());
                SendMessage(s_wcd.hwndInputLine, EM_SETSEL, (WPARAM)s_wcd.consoleField.GetCursor(), (LPARAM)s_wcd.consoleField.GetCursor());
                return 0;
            }

            if (key == SC_DOWNARROW || key == SC_KP_2)
            {
                if (s_wcd.historyLine == s_wcd.nextHistoryLine)
                {
                    return 0;
                }
                s_wcd.historyLine++;
                s_wcd.consoleField = s_wcd.historyEditLines[s_wcd.historyLine % COMMAND_HISTORY];

                SetWindowText(s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer());
                SendMessage(s_wcd.hwndInputLine, EM_SETSEL, (WPARAM)s_wcd.consoleField.GetCursor(), (LPARAM)s_wcd.consoleField.GetCursor());
                return 0;
            }
            break;

        case WM_CHAR:
            key = ((lParam >> 16) & 0xFF) | (((lParam >> 24) & 1) << 7);

            GetWindowText(s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer(), DN_MAX_EDIT_LINE);
            SendMessage(s_wcd.hwndInputLine, EM_GETSEL, (WPARAM)NULL, (LPARAM)&cursor);
            s_wcd.consoleField.SetCursor(cursor);

            // enter the line
            if (key == SC_ENTER)
            {
                // Qcommon_Frame adds the newline when it executes the text
                strncat(s_wcd.consoleText, s_wcd.consoleField.GetBuffer(),
                        sizeof(s_wcd.consoleText) - strlen(s_wcd.consoleText) - 1);
                SetWindowText(s_wcd.hwndInputLine, "");

                Sys_ConsoleOutput(va("]%s\n", s_wcd.consoleField.GetBuffer()));

                // copy line to history buffer
                s_wcd.historyEditLines[s_wcd.nextHistoryLine % COMMAND_HISTORY] = s_wcd.consoleField;
                s_wcd.nextHistoryLine++;
                s_wcd.historyLine = s_wcd.nextHistoryLine;

                s_wcd.consoleField.Clear();

                return 0;
            }

            // command completion
            if (key == SC_TAB)
            {
                s_wcd.consoleField.AutoComplete();

                SetWindowText(s_wcd.hwndInputLine, s_wcd.consoleField.GetBuffer());
                SendMessage(s_wcd.hwndInputLine, EM_SETSEL, (WPARAM)s_wcd.consoleField.GetCursor(), (LPARAM)s_wcd.consoleField.GetCursor());

                return 0;
            }

            // clear autocompletion on any other key input
            s_wcd.consoleField.ClearAutoComplete();
            break;
    }

    return CallWindowProc(s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam);
}

/*
================
Sys_CreateConsole
================
*/
void Sys_CreateConsole(void)
{
    HDC hDC;
    WNDCLASS wc;
    RECT rect;
    const char *DEDCLASS = CONSOLE_WINDOW_CLASS;
    int nHeight;
    int swidth, sheight;
    int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;
    int i;

    memset(&wc, 0, sizeof(wc));

    s_wcd.hInstance = GetModuleHandle(NULL);

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)ConWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = s_wcd.hInstance;
    wc.hIcon = LoadIcon(s_wcd.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = 0;
    wc.lpszClassName = DEDCLASS;

    if (!RegisterClass(&wc))
    {
        return;
    }

    rect.left = 0;
    rect.right = 540;
    rect.top = 0;
    rect.bottom = 450;
    AdjustWindowRect(&rect, DEDSTYLE, FALSE);

    hDC = GetDC(GetDesktopWindow());
    swidth = GetDeviceCaps(hDC, HORZRES);
    sheight = GetDeviceCaps(hDC, VERTRES);
    ReleaseDC(GetDesktopWindow(), hDC);

    s_wcd.windowWidth = rect.right - rect.left + 1;
    s_wcd.windowHeight = rect.bottom - rect.top + 1;

    s_wcd.hWnd = CreateWindowEx(0,
                                DEDCLASS,
                                CONSOLE_WINDOW_TITLE,
                                DEDSTYLE,
                                (swidth - 600) / 2, (sheight - 450) / 2, rect.right - rect.left + 1, rect.bottom - rect.top + 1,
                                NULL,
                                NULL,
                                s_wcd.hInstance,
                                NULL);

    if (s_wcd.hWnd == NULL)
    {
        return;
    }

    //
    // create fonts
    //
    hDC = GetDC(s_wcd.hWnd);
    nHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);

    s_wcd.hfBufferFont = CreateFont(nHeight, 0, 0, 0, FW_LIGHT, 0, 0, 0, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                    FF_MODERN | FIXED_PITCH, "Courier New");

    ReleaseDC(s_wcd.hWnd, hDC);

    //
    // create the input line
    //
    s_wcd.hwndInputLine = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
                                            ES_LEFT | ES_AUTOHSCROLL,
                                            6, 400, 528, 20,
                                            s_wcd.hWnd,
                                            (HMENU)INPUT_ID,   // child window ID
                                            s_wcd.hInstance, NULL);

    //
    // create the buttons
    //
    s_wcd.hwndButtonCopy = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                            5, 425, 72, 24,
                                            s_wcd.hWnd,
                                            (HMENU)COPY_ID,    // child window ID
                                            s_wcd.hInstance, NULL);
    SendMessage(s_wcd.hwndButtonCopy, WM_SETTEXT, 0, (LPARAM)"copy");

    s_wcd.hwndButtonClear = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                            82, 425, 72, 24,
                                            s_wcd.hWnd,
                                            (HMENU)CLEAR_ID,   // child window ID
                                            s_wcd.hInstance, NULL);
    SendMessage(s_wcd.hwndButtonClear, WM_SETTEXT, 0, (LPARAM)"clear");

    s_wcd.hwndButtonQuit = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                            462, 425, 72, 24,
                                            s_wcd.hWnd,
                                            (HMENU)QUIT_ID,    // child window ID
                                            s_wcd.hInstance, NULL);
    SendMessage(s_wcd.hwndButtonQuit, WM_SETTEXT, 0, (LPARAM)"quit");

    //
    // create the scrollbuffer
    //
    s_wcd.hwndBuffer = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
                                            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                            6, 40, 526, 354,
                                            s_wcd.hWnd,
                                            (HMENU)EDIT_ID,    // child window ID
                                            s_wcd.hInstance, NULL);
    SendMessage(s_wcd.hwndBuffer, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);

    s_wcd.SysInputLineWndProc = (WNDPROC)SetWindowLong(s_wcd.hwndInputLine, GWL_WNDPROC, (long)InputLineWndProc);
    SendMessage(s_wcd.hwndInputLine, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);

    // stays hidden until win_viewlog or a dedicated server shows it
    s_wcd.consoleField.Clear();

    for (i = 0; i < COMMAND_HISTORY; i++)
    {
        s_wcd.historyEditLines[i].Clear();
    }
}

/*
================
Sys_DestroyConsole
================
*/
void Sys_DestroyConsole(void)
{
    if (s_wcd.hWnd)
    {
        ShowWindow(s_wcd.hWnd, SW_HIDE);
        CloseWindow(s_wcd.hWnd);
        DestroyWindow(s_wcd.hWnd);
        s_wcd.hWnd = 0;
    }
}

/*
================
Sys_ShowConsole
================
*/
void Sys_ShowConsole(int visLevel, qboolean quitOnClose)
{
    s_wcd.quitOnClose = quitOnClose;

    if (!s_wcd.hWnd)
    {
        return;
    }

    switch (visLevel)
    {
        case 0:
            ShowWindow(s_wcd.hWnd, SW_HIDE);
            break;
        case 1:
            ShowWindow(s_wcd.hWnd, SW_SHOWNORMAL);
            SendMessage(s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff);
            break;
        case 2:
            ShowWindow(s_wcd.hWnd, SW_MINIMIZE);
            break;
        default:
            Sys_Error("Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel);
            break;
    }
}

/*
================
Sys_ConsoleCreated
================
*/
qboolean Sys_ConsoleCreated(void)
{
    return (qboolean)(s_wcd.hWnd != NULL);
}

/*
================
Sys_SysconInput
================
*/
char *Sys_SysconInput(void)
{
    if (s_wcd.consoleText[0] == 0)
    {
        return NULL;
    }

    strcpy(s_wcd.returnedText, s_wcd.consoleText);
    s_wcd.consoleText[0] = 0;

    return s_wcd.returnedText;
}

/*
================
Conbuf_AppendText
================
*/
void Conbuf_AppendText(const char *pMsg)
{
#define CONSOLE_BUFFER_SIZE 16384

    char buffer[CONSOLE_BUFFER_SIZE * 2];
    char *b = buffer;
    const char *msg;
    int bufLen;
    int i = 0;
    static unsigned long s_totalChars;

    //
    // if the message is REALLY long, use just the last portion of it
    //
    if (strlen(pMsg) > CONSOLE_BUFFER_SIZE - 1)
    {
        msg = pMsg + strlen(pMsg) - CONSOLE_BUFFER_SIZE + 1;
    }
    else
    {
        msg = pMsg;
    }

    //
    // copy into an intermediate buffer
    //
    while (msg[i] && ((b - buffer) < sizeof(buffer) - 1))
    {
        if (msg[i] == '\n' && msg[i + 1] == '\r')
        {
            b[0] = '\r';
            b[1] = '\n';
            b += 2;
            i++;
        }
        else if (msg[i] == '\r')
        {
            b[0] = '\r';
            b[1] = '\n';
            b += 2;
        }
        else if (msg[i] == '\n')
        {
            b[0] = '\r';
            b[1] = '\n';
            b += 2;
        }
        else
        {
            // Q2 carries console color in the character high bit, which
            // the win32 edit control cannot display; the BFG color code
            // strip became this mask
            *b = msg[i] & 0x7f;
            b++;
        }
        i++;
    }
    *b = 0;
    bufLen = b - buffer;

    s_totalChars += bufLen;

    //
    // replace selection instead of appending if we're overflowing
    //
    if (s_totalChars > 0x7000)
    {
        SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
        s_totalChars = bufLen;
    }

    //
    // put this text into the windows console
    //
    SendMessage(s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff);
    SendMessage(s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0);
    SendMessage(s_wcd.hwndBuffer, EM_REPLACESEL, 0, (LPARAM)buffer);
}

/*
================
Win_SetErrorText
================
*/
void Win_SetErrorText(const char *buf)
{
    if (!s_wcd.hWnd)
    {
        return;
    }

    strncpy(s_wcd.errorString, buf, sizeof(s_wcd.errorString) - 1);
    s_wcd.errorString[sizeof(s_wcd.errorString) - 1] = 0;
    if (!s_wcd.hwndErrorBox)
    {
        s_wcd.hwndErrorBox = CreateWindow("static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
                                            6, 5, 526, 30,
                                            s_wcd.hWnd,
                                            (HMENU)ERRORBOX_ID,   // child window ID
                                            s_wcd.hInstance, NULL);
        SendMessage(s_wcd.hwndErrorBox, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);
        SetWindowText(s_wcd.hwndErrorBox, s_wcd.errorString);

        DestroyWindow(s_wcd.hwndInputLine);
        s_wcd.hwndInputLine = NULL;
    }
}
