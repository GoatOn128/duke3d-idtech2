//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "keyboard.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
#include <string.h>

/*
=============================================================================

                               GLOBAL VARIABLES

=============================================================================
*/

byte KB_KeyDown[MAXKEYBOARDSCAN];
kb_scancode KB_LastScan;

static volatile boolean keyIsWaiting = 0;

static uint8_t scancodeToASCII[MAXKEYBOARDSCAN];
static uint8_t shiftedScancodeToASCII[MAXKEYBOARDSCAN];

static uint8_t vkToSC[256];

static uint8_t keyQueue[128];
static int keyQueueHead = 0;
static int keyQueueTail = 0;

static int keypadActive = 0;

/*
=============================================================================

                               INTERNAL HELPERS

=============================================================================
*/

static void KB_QueueByte(uint8_t ch)
{
    int next = (keyQueueHead + 1) & 127;

    if (next == keyQueueTail)
        return;

    keyQueue[keyQueueHead] = ch;
    keyQueueHead = next;
    keyIsWaiting = true;
}

static uint8_t KB_DequeueByte(void)
{
    uint8_t ch;

    if (keyQueueHead == keyQueueTail)
        return 0;

    ch = keyQueue[keyQueueTail];
    keyQueueTail = (keyQueueTail + 1) & 127;

    if (keyQueueHead == keyQueueTail)
        keyIsWaiting = false;

    return ch;
}

static int KB_QueueHasData(void)
{
    return keyQueueHead != keyQueueTail;
}

static void KB_ClearQueue(void)
{
    int i;

    for (i = 0; i < MAXKEYBOARDSCAN; i++)
    {
        if (KB_KeyDown[i])
        {
            KB_KeyDown[i] = 0;
            CONTROL_UpdateKeyboardState(i, 0);
        }
    }
    keyQueueHead = 0;
    keyQueueTail = 0;
    keyIsWaiting = false;
}

static void KB_MapVK(int vk, kb_scancode sc)
{
    if (vk >= 0 && vk < 256)
        vkToSC[vk] = (uint8_t)sc;
}

static int KB_IsExtendedKey(LPARAM lParam)
{
    return (lParam & 0x01000000) != 0;
}

static int KB_IsShiftDown(void)
{
    return KB_KeyDown[sc_LeftShift] || KB_KeyDown[sc_RightShift];
}

static uint8_t KB_ScanToASCII(kb_scancode sc)
{
    if (sc <= 0 || sc >= MAXKEYBOARDSCAN)
        return 0xFF;

    if (KB_IsShiftDown())
        return shiftedScancodeToASCII[sc];

    return scancodeToASCII[sc];
}

static kb_scancode KB_MapWin32Key(WPARAM wParam, LPARAM lParam)
{
    UINT vk = (UINT)wParam;
    UINT scan = (UINT)((lParam >> 16) & 0xff);
    int extended = KB_IsExtendedKey(lParam);

    switch (vk)
    {
    case VK_SHIFT:
    {
        UINT leftShiftScan = MapVirtualKeyA(VK_LSHIFT, MAPVK_VK_TO_VSC);
        return (scan == leftShiftScan) ? sc_LeftShift : sc_RightShift;
    }

    case VK_CONTROL:
        return extended ? sc_RightControl : sc_LeftControl;

    case VK_MENU:
        return extended ? sc_RightAlt : sc_LeftAlt;

    case VK_RETURN:
        return extended ? sc_kpad_Enter : sc_Enter;

    case VK_INSERT:
        return extended ? sc_Insert : sc_kpad_0;

    case VK_DELETE:
        return extended ? sc_Delete : sc_kpad_Period;

    case VK_HOME:
        return extended ? sc_Home : sc_kpad_7;

    case VK_END:
        return extended ? sc_End : sc_kpad_1;

    case VK_PRIOR:
        return extended ? sc_PgUp : sc_kpad_9;

    case VK_NEXT:
        return extended ? sc_PgDn : sc_kpad_3;

    case VK_LEFT:
        return extended ? sc_LeftArrow : sc_kpad_4;

    case VK_RIGHT:
        return extended ? sc_RightArrow : sc_kpad_6;

    case VK_UP:
        return extended ? sc_UpArrow : sc_kpad_8;

    case VK_DOWN:
        return extended ? sc_DownArrow : sc_kpad_2;

    case VK_DIVIDE:
        return sc_kpad_Slash;

    case VK_SNAPSHOT:
        return sc_PrintScreen;

    default:
        break;
    }

    if (vk < 256)
        return (kb_scancode)vkToSC[vk];

    return (kb_scancode)0;
}

static void KB_PumpMessages(void)
{
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            PostQuitMessage((int)msg.wParam);
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

/*
=============================================================================

                               WNDPROC HOOK

=============================================================================
*/

static void KB_KeyEventFromWin32(WPARAM wParam, LPARAM lParam, int pressed)
{
    kb_scancode sc = KB_MapWin32Key(wParam, lParam);

    if (sc == 0)
        return;

    KB_KeyEvent((int)sc, pressed);
}

/*
Call this from your real Win32 WndProc, or use it directly as the WndProc.

Example:

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_CHAR:
        case WM_ACTIVATEAPP:
        case WM_KILLFOCUS:
            return Duke3D_KeyboardWndProc(hWnd, msg, wParam, lParam);
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}
*/

LRESULT CALLBACK Engine_KeyboardWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        int wasDown = (lParam & 0x40000000) != 0;

        if (!wasDown)
            KB_KeyEventFromWin32(wParam, lParam, 1);

        return 0;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        KB_KeyEventFromWin32(wParam, lParam, 0);
        return 0;
    }

    case WM_CHAR:
    {
        if (wParam >= 1 && wParam <= 255)
            KB_QueueByte((uint8_t)wParam);

        return 0;
    }

    case WM_ACTIVATEAPP:
    {
        if (!wParam)
            KB_ClearKeysDown();

        break;
    }

    case WM_KILLFOCUS:
    {
        KB_ClearKeysDown();
        break;
    }

    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

/*
=============================================================================

                               PUBLIC FUNCTIONS

=============================================================================
*/

uint8_t _readlastkeyhit(void)
{
    KB_PumpMessages();
    return KB_DequeueByte();
}

void keyhandler(void)
{
    KB_PumpMessages();
}

void KB_KeyEvent(int scancode, int keypressed)
{
    kb_scancode sc = (kb_scancode)scancode;
    uint8_t ascii;

    if (sc <= 0 || sc >= MAXKEYBOARDSCAN)
        return;

    keypressed = keypressed ? 1 : 0;

    if (KB_KeyDown[sc] == keypressed)
        return;

    KB_KeyDown[sc] = (byte)keypressed;

    if (keypressed)
    {
        KB_LastScan = sc;
        keyIsWaiting = true;

        ascii = KB_ScanToASCII(sc);

        if (ascii != 0xFF && ascii != 0)
            KB_QueueByte(ascii);
    }

    CONTROL_UpdateKeyboardState(sc, keypressed);
}

int KB_KeyWaiting(void)
{
    KB_PumpMessages();
    return keyIsWaiting || KB_QueueHasData();
}

uint8_t KB_Getch(void)
{
    uint8_t ch;

    while (!KB_KeyWaiting())
        Sleep(1);

    ch = KB_DequeueByte();

    if (ch != 0)
        return ch;

    if (KB_LastScan >= MAXKEYBOARDSCAN)
        return 0xFF;

    keyIsWaiting = false;
    return KB_ScanToASCII(KB_LastScan);
}

void KB_Addch(uint8_t ch)
{
    KB_QueueByte(ch);
}

void KB_FlushKeyboardQueue(void)
{
    KB_ClearQueue();
}

void KB_ClearKeysDown(void)
{
    int i;

    for (i = 0; i < MAXKEYBOARDSCAN; i++)
    {
        if (KB_KeyDown[i])
        {
            KB_KeyDown[i] = 0;
            CONTROL_UpdateKeyboardState(i, 0);
        }
    }

    KB_LastScan = 0;
    KB_ClearQueue();
}

/*
=============================================================================

                               SCANCODE NAMES

=============================================================================
*/

static struct
{
    const char* name;
    int code;
} keyname2scancode[] =
{
    { ",",      sc_Comma        },
    { ".",      sc_Period       },
    { "Enter",  sc_Return       },
    { "Escape", sc_Escape       },
    { "Space",  sc_Space        },
    { "BakSpc", sc_BackSpace    },
    { "Tab",    sc_Tab          },
    { "LAlt",   sc_LeftAlt      },
    { "LCtrl",  sc_LeftControl  },
    { "CapLck", sc_CapsLock     },
    { "LShift", sc_LeftShift    },
    { "RShift", sc_RightShift   },
    { "F1",     sc_F1           },
    { "F2",     sc_F2           },
    { "F3",     sc_F3           },
    { "F4",     sc_F4           },
    { "F5",     sc_F5           },
    { "F6",     sc_F6           },
    { "F7",     sc_F7           },
    { "F8",     sc_F8           },
    { "F9",     sc_F9           },
    { "F10",    sc_F10          },
    { "F11",    sc_F11          },
    { "F12",    sc_F12          },
    { "Kpad*",  sc_Kpad_Star    },
    { "Pause",  sc_Pause        },
    { "ScrLck", sc_ScrollLock   },
    { "NumLck", sc_NumLock      },
    { "/",      sc_Slash        },
    { ";",      sc_SemiColon    },
    { "'",      sc_Quote        },
    { "`",      sc_Tilde        },
    { "\\",     sc_BackSlash    },
    { "[",      sc_OpenBracket  },
    { "]",      sc_CloseBracket },
    { "1",      sc_1            },
    { "2",      sc_2            },
    { "3",      sc_3            },
    { "4",      sc_4            },
    { "5",      sc_5            },
    { "6",      sc_6            },
    { "7",      sc_7            },
    { "8",      sc_8            },
    { "9",      sc_9            },
    { "0",      sc_0            },
    { "-",      sc_Minus        },
    { "=",      sc_Equals       },
    { "+",      sc_Plus         },
    { "Kpad1",  sc_kpad_1       },
    { "Kpad2",  sc_kpad_2       },
    { "Kpad3",  sc_kpad_3       },
    { "Kpad4",  sc_kpad_4       },
    { "Kpad5",  sc_kpad_5       },
    { "Kpad6",  sc_kpad_6       },
    { "Kpad7",  sc_kpad_7       },
    { "Kpad8",  sc_kpad_8       },
    { "Kpad9",  sc_kpad_9       },
    { "Kpad0",  sc_kpad_0       },
    { "Kpad-",  sc_kpad_Minus   },
    { "Kpad+",  sc_kpad_Plus    },
    { "Kpad.",  sc_kpad_Period  },
    { "A",      sc_A            },
    { "B",      sc_B            },
    { "C",      sc_C            },
    { "D",      sc_D            },
    { "E",      sc_E            },
    { "F",      sc_F            },
    { "G",      sc_G            },
    { "H",      sc_H            },
    { "I",      sc_I            },
    { "J",      sc_J            },
    { "K",      sc_K            },
    { "L",      sc_L            },
    { "M",      sc_M            },
    { "N",      sc_N            },
    { "O",      sc_O            },
    { "P",      sc_P            },
    { "Q",      sc_Q            },
    { "R",      sc_R            },
    { "S",      sc_S            },
    { "T",      sc_T            },
    { "U",      sc_U            },
    { "V",      sc_V            },
    { "W",      sc_W            },
    { "X",      sc_X            },
    { "Y",      sc_Y            },
    { "Z",      sc_Z            },
    { "Up",     sc_UpArrow      },
    { "Down",   sc_DownArrow    },
    { "Left",   sc_LeftArrow    },
    { "Right",  sc_RightArrow   },
    { "Insert", sc_Insert       },
    { "Delete", sc_Delete       },
    { "Home",   sc_Home         },
    { "End",    sc_End          },
    { "PgUp",   sc_PgUp         },
    { "PgDn",   sc_PgDn         },
    { "RAlt",   sc_RightAlt     },
    { "RCtrl",  sc_RightControl },
    { "Kpad/",  sc_kpad_Slash   },
    { "KpdEnt", sc_kpad_Enter   },
    { "PrtScn", sc_PrintScreen  },
    { NULL,     0               }
};

char* KB_ScanCodeToString(kb_scancode scancode)
{
    int i;

    for (i = 0; keyname2scancode[i].name != NULL; i++)
    {
        if (keyname2scancode[i].code == scancode)
            return (char*)keyname2scancode[i].name;
    }

    return NULL;
}

kb_scancode KB_StringToScanCode(char* string)
{
    int32 i = 0;

    if (string == NULL)
        return (kb_scancode)0;

    while (keyname2scancode[i].name != NULL)
    {
        if (_stricmp(keyname2scancode[i].name, string) == 0)
            return (kb_scancode)keyname2scancode[i].code;

        i++;
    }

    return (kb_scancode)0;
}

/*
=============================================================================

                               KEYPAD

=============================================================================
*/

void KB_TurnKeypadOn(void)
{
    keypadActive = 1;
}

void KB_TurnKeypadOff(void)
{
    keypadActive = 0;
}

int KB_KeypadActive(void)
{
    return keypadActive;
}

/*
=============================================================================

                               STARTUP / SHUTDOWN

=============================================================================
*/

void KB_Startup(void)
{
    memset(KB_KeyDown, 0, sizeof(KB_KeyDown));
    memset(vkToSC, 0, sizeof(vkToSC));
    memset(scancodeToASCII, 0xFF, sizeof(scancodeToASCII));
    memset(shiftedScancodeToASCII, 0xFF, sizeof(shiftedScancodeToASCII));

    KB_LastScan = 0;
    keyIsWaiting = false;
    keyQueueHead = 0;
    keyQueueTail = 0;
    keypadActive = 0;

    /*
    -------------------------------------------------------------------------
    Win32 VK -> Duke scan code table
    -------------------------------------------------------------------------
    */

    KB_MapVK(VK_ESCAPE, sc_Escape);
    KB_MapVK(VK_TAB, sc_Tab);
    KB_MapVK(VK_SPACE, sc_Space);
    KB_MapVK(VK_BACK, sc_BackSpace);
    KB_MapVK(VK_CAPITAL, sc_CapsLock);
    KB_MapVK(VK_PAUSE, sc_Pause);
    KB_MapVK(VK_SCROLL, sc_ScrollLock);
    KB_MapVK(VK_NUMLOCK, sc_NumLock);

    KB_MapVK('A', sc_A);
    KB_MapVK('B', sc_B);
    KB_MapVK('C', sc_C);
    KB_MapVK('D', sc_D);
    KB_MapVK('E', sc_E);
    KB_MapVK('F', sc_F);
    KB_MapVK('G', sc_G);
    KB_MapVK('H', sc_H);
    KB_MapVK('I', sc_I);
    KB_MapVK('J', sc_J);
    KB_MapVK('K', sc_K);
    KB_MapVK('L', sc_L);
    KB_MapVK('M', sc_M);
    KB_MapVK('N', sc_N);
    KB_MapVK('O', sc_O);
    KB_MapVK('P', sc_P);
    KB_MapVK('Q', sc_Q);
    KB_MapVK('R', sc_R);
    KB_MapVK('S', sc_S);
    KB_MapVK('T', sc_T);
    KB_MapVK('U', sc_U);
    KB_MapVK('V', sc_V);
    KB_MapVK('W', sc_W);
    KB_MapVK('X', sc_X);
    KB_MapVK('Y', sc_Y);
    KB_MapVK('Z', sc_Z);

    KB_MapVK('0', sc_0);
    KB_MapVK('1', sc_1);
    KB_MapVK('2', sc_2);
    KB_MapVK('3', sc_3);
    KB_MapVK('4', sc_4);
    KB_MapVK('5', sc_5);
    KB_MapVK('6', sc_6);
    KB_MapVK('7', sc_7);
    KB_MapVK('8', sc_8);
    KB_MapVK('9', sc_9);

    KB_MapVK(VK_OEM_COMMA, sc_Comma);
    KB_MapVK(VK_OEM_PERIOD, sc_Period);
    KB_MapVK(VK_OEM_2, sc_Slash);
    KB_MapVK(VK_OEM_1, sc_SemiColon);
    KB_MapVK(VK_OEM_7, sc_Quote);
    KB_MapVK(VK_OEM_3, sc_Tilde);
    KB_MapVK(VK_OEM_5, sc_BackSlash);
    KB_MapVK(VK_OEM_4, sc_OpenBracket);
    KB_MapVK(VK_OEM_6, sc_CloseBracket);
    KB_MapVK(VK_OEM_MINUS, sc_Minus);
    KB_MapVK(VK_OEM_PLUS, sc_Equals);

    KB_MapVK(VK_F1, sc_F1);
    KB_MapVK(VK_F2, sc_F2);
    KB_MapVK(VK_F3, sc_F3);
    KB_MapVK(VK_F4, sc_F4);
    KB_MapVK(VK_F5, sc_F5);
    KB_MapVK(VK_F6, sc_F6);
    KB_MapVK(VK_F7, sc_F7);
    KB_MapVK(VK_F8, sc_F8);
    KB_MapVK(VK_F9, sc_F9);
    KB_MapVK(VK_F10, sc_F10);
    KB_MapVK(VK_F11, sc_F11);
    KB_MapVK(VK_F12, sc_F12);

    KB_MapVK(VK_NUMPAD0, sc_kpad_0);
    KB_MapVK(VK_NUMPAD1, sc_kpad_1);
    KB_MapVK(VK_NUMPAD2, sc_kpad_2);
    KB_MapVK(VK_NUMPAD3, sc_kpad_3);
    KB_MapVK(VK_NUMPAD4, sc_kpad_4);
    KB_MapVK(VK_NUMPAD5, sc_kpad_5);
    KB_MapVK(VK_NUMPAD6, sc_kpad_6);
    KB_MapVK(VK_NUMPAD7, sc_kpad_7);
    KB_MapVK(VK_NUMPAD8, sc_kpad_8);
    KB_MapVK(VK_NUMPAD9, sc_kpad_9);
    KB_MapVK(VK_MULTIPLY, sc_Kpad_Star);
    KB_MapVK(VK_SUBTRACT, sc_kpad_Minus);
    KB_MapVK(VK_ADD, sc_kpad_Plus);
    KB_MapVK(VK_DECIMAL, sc_kpad_Period);
    KB_MapVK(VK_DIVIDE, sc_kpad_Slash);

    /*
    -------------------------------------------------------------------------
    Normal ASCII table
    -------------------------------------------------------------------------
    */

    scancodeToASCII[sc_A] = 'a';
    scancodeToASCII[sc_B] = 'b';
    scancodeToASCII[sc_C] = 'c';
    scancodeToASCII[sc_D] = 'd';
    scancodeToASCII[sc_E] = 'e';
    scancodeToASCII[sc_F] = 'f';
    scancodeToASCII[sc_G] = 'g';
    scancodeToASCII[sc_H] = 'h';
    scancodeToASCII[sc_I] = 'i';
    scancodeToASCII[sc_J] = 'j';
    scancodeToASCII[sc_K] = 'k';
    scancodeToASCII[sc_L] = 'l';
    scancodeToASCII[sc_M] = 'm';
    scancodeToASCII[sc_N] = 'n';
    scancodeToASCII[sc_O] = 'o';
    scancodeToASCII[sc_P] = 'p';
    scancodeToASCII[sc_Q] = 'q';
    scancodeToASCII[sc_R] = 'r';
    scancodeToASCII[sc_S] = 's';
    scancodeToASCII[sc_T] = 't';
    scancodeToASCII[sc_U] = 'u';
    scancodeToASCII[sc_V] = 'v';
    scancodeToASCII[sc_W] = 'w';
    scancodeToASCII[sc_X] = 'x';
    scancodeToASCII[sc_Y] = 'y';
    scancodeToASCII[sc_Z] = 'z';

    scancodeToASCII[sc_0] = '0';
    scancodeToASCII[sc_1] = '1';
    scancodeToASCII[sc_2] = '2';
    scancodeToASCII[sc_3] = '3';
    scancodeToASCII[sc_4] = '4';
    scancodeToASCII[sc_5] = '5';
    scancodeToASCII[sc_6] = '6';
    scancodeToASCII[sc_7] = '7';
    scancodeToASCII[sc_8] = '8';
    scancodeToASCII[sc_9] = '9';

    scancodeToASCII[sc_Escape] = asc_Escape;
    scancodeToASCII[sc_Tab] = asc_Tab;
    scancodeToASCII[sc_Space] = asc_Space;
    scancodeToASCII[sc_Enter] = asc_Enter;
    scancodeToASCII[sc_Return] = asc_Enter;
    scancodeToASCII[sc_BackSpace] = asc_BackSpace;

    scancodeToASCII[sc_Comma] = ',';
    scancodeToASCII[sc_Period] = '.';
    scancodeToASCII[sc_Kpad_Star] = '*';
    scancodeToASCII[sc_Slash] = '/';
    scancodeToASCII[sc_SemiColon] = ';';
    scancodeToASCII[sc_Quote] = '\'';
    scancodeToASCII[sc_Tilde] = '`';
    scancodeToASCII[sc_BackSlash] = '\\';
    scancodeToASCII[sc_OpenBracket] = '[';
    scancodeToASCII[sc_CloseBracket] = ']';
    scancodeToASCII[sc_Minus] = '-';
    scancodeToASCII[sc_Equals] = '=';
    scancodeToASCII[sc_Plus] = '+';

    scancodeToASCII[sc_kpad_0] = '0';
    scancodeToASCII[sc_kpad_1] = '1';
    scancodeToASCII[sc_kpad_2] = '2';
    scancodeToASCII[sc_kpad_3] = '3';
    scancodeToASCII[sc_kpad_4] = '4';
    scancodeToASCII[sc_kpad_5] = '5';
    scancodeToASCII[sc_kpad_6] = '6';
    scancodeToASCII[sc_kpad_7] = '7';
    scancodeToASCII[sc_kpad_8] = '8';
    scancodeToASCII[sc_kpad_9] = '9';
    scancodeToASCII[sc_kpad_Minus] = '-';
    scancodeToASCII[sc_kpad_Period] = '.';
    scancodeToASCII[sc_kpad_Plus] = '+';
    scancodeToASCII[sc_kpad_Slash] = '/';
    scancodeToASCII[sc_kpad_Enter] = asc_Enter;

    /*
    -------------------------------------------------------------------------
    Shifted ASCII table
    -------------------------------------------------------------------------
    */

    shiftedScancodeToASCII[sc_A] = 'A';
    shiftedScancodeToASCII[sc_B] = 'B';
    shiftedScancodeToASCII[sc_C] = 'C';
    shiftedScancodeToASCII[sc_D] = 'D';
    shiftedScancodeToASCII[sc_E] = 'E';
    shiftedScancodeToASCII[sc_F] = 'F';
    shiftedScancodeToASCII[sc_G] = 'G';
    shiftedScancodeToASCII[sc_H] = 'H';
    shiftedScancodeToASCII[sc_I] = 'I';
    shiftedScancodeToASCII[sc_J] = 'J';
    shiftedScancodeToASCII[sc_K] = 'K';
    shiftedScancodeToASCII[sc_L] = 'L';
    shiftedScancodeToASCII[sc_M] = 'M';
    shiftedScancodeToASCII[sc_N] = 'N';
    shiftedScancodeToASCII[sc_O] = 'O';
    shiftedScancodeToASCII[sc_P] = 'P';
    shiftedScancodeToASCII[sc_Q] = 'Q';
    shiftedScancodeToASCII[sc_R] = 'R';
    shiftedScancodeToASCII[sc_S] = 'S';
    shiftedScancodeToASCII[sc_T] = 'T';
    shiftedScancodeToASCII[sc_U] = 'U';
    shiftedScancodeToASCII[sc_V] = 'V';
    shiftedScancodeToASCII[sc_W] = 'W';
    shiftedScancodeToASCII[sc_X] = 'X';
    shiftedScancodeToASCII[sc_Y] = 'Y';
    shiftedScancodeToASCII[sc_Z] = 'Z';

    shiftedScancodeToASCII[sc_0] = ')';
    shiftedScancodeToASCII[sc_1] = '!';
    shiftedScancodeToASCII[sc_2] = '@';
    shiftedScancodeToASCII[sc_3] = '#';
    shiftedScancodeToASCII[sc_4] = '$';
    shiftedScancodeToASCII[sc_5] = '%';
    shiftedScancodeToASCII[sc_6] = '^';
    shiftedScancodeToASCII[sc_7] = '&';
    shiftedScancodeToASCII[sc_8] = '*';
    shiftedScancodeToASCII[sc_9] = '(';

    shiftedScancodeToASCII[sc_Escape] = asc_Escape;
    shiftedScancodeToASCII[sc_Tab] = asc_Tab;
    shiftedScancodeToASCII[sc_Space] = asc_Space;
    shiftedScancodeToASCII[sc_Enter] = asc_Enter;
    shiftedScancodeToASCII[sc_Return] = asc_Enter;
    shiftedScancodeToASCII[sc_BackSpace] = asc_BackSpace;

    shiftedScancodeToASCII[sc_Comma] = '<';
    shiftedScancodeToASCII[sc_Period] = '>';
    shiftedScancodeToASCII[sc_Kpad_Star] = '*';
    shiftedScancodeToASCII[sc_Slash] = '?';
    shiftedScancodeToASCII[sc_SemiColon] = ':';
    shiftedScancodeToASCII[sc_Quote] = '"';
    shiftedScancodeToASCII[sc_Tilde] = '~';
    shiftedScancodeToASCII[sc_BackSlash] = '|';
    shiftedScancodeToASCII[sc_OpenBracket] = '{';
    shiftedScancodeToASCII[sc_CloseBracket] = '}';
    shiftedScancodeToASCII[sc_Minus] = '_';
    shiftedScancodeToASCII[sc_Equals] = '+';
    shiftedScancodeToASCII[sc_Plus] = '+';

    shiftedScancodeToASCII[sc_kpad_0] = '0';
    shiftedScancodeToASCII[sc_kpad_1] = '1';
    shiftedScancodeToASCII[sc_kpad_2] = '2';
    shiftedScancodeToASCII[sc_kpad_3] = '3';
    shiftedScancodeToASCII[sc_kpad_4] = '4';
    shiftedScancodeToASCII[sc_kpad_5] = '5';
    shiftedScancodeToASCII[sc_kpad_6] = '6';
    shiftedScancodeToASCII[sc_kpad_7] = '7';
    shiftedScancodeToASCII[sc_kpad_8] = '8';
    shiftedScancodeToASCII[sc_kpad_9] = '9';
    shiftedScancodeToASCII[sc_kpad_Minus] = '-';
    shiftedScancodeToASCII[sc_kpad_Period] = '.';
    shiftedScancodeToASCII[sc_kpad_Plus] = '+';
    shiftedScancodeToASCII[sc_kpad_Slash] = '/';
    shiftedScancodeToASCII[sc_kpad_Enter] = asc_Enter;

    KB_ClearKeysDown();
}

void KB_Shutdown(void)
{
    KB_ClearKeysDown();
}