/* MENUES.cpp's ten named slots and last-slot quick save/load, using Quake's
   server persistence commands and the existing Duke menu/input stack. */
#include "client.h"
#include <ctype.h>

#define DN_SAVE_SLOTS 10
#define DN_SAVE_NAME 19

void M_PushMenu(void (*draw)(void), const char *(*key)(int key));
void M_PopMenu(void);

static char descriptions[DN_SAVE_SLOTS + 1][32];
static qboolean valid[DN_SAVE_SLOTS + 1];
static int cursor;
static int confirm_choice;
static qboolean saving, editing, confirming, quick;
static char description[DN_SAVE_NAME + 1];

/* Original GAME.cpp minitext uses four-pixel advances (five for spaces).
   The large menu alphabet is taller than the original twelve-pixel rows. */
static void SmallText(int x, int y, const char *text)
{
    if (x == 160)
    {
        int width = 0;
        for (const char *c = text; *c; c++)
        {
            width += *c == ' ' ? 5 : 4;
        }
        x -= width / 2;
    }
    for (const unsigned char *c = (const unsigned char *)text; *c; c++)
    {
        int glyph = toupper(*c);
        if (glyph >= '!' && glyph <= 'Z')
        {
            DN_DrawUiTile(x, y, 3072 + glyph - '!', 0, 0, 1);
        }
        x += glyph == ' ' ? 5 : 4;
    }
}

static void ReadSlots(void)
{
    for (int i = 0; i <= DN_SAVE_SLOTS; i++)
    {
        char path[MAX_OSPATH];
        Com_sprintf(path, sizeof(path), "%s/save/save%d/server.ssv", FS_Gamedir(), i);
        FILE *file = fopen(path, "rb");
        valid[i] = false;
        if (file)
        {
            valid[i] = fread(descriptions[i], 1, sizeof(descriptions[i]), file) ==
                           sizeof(descriptions[i]) &&
                       memchr(descriptions[i], 0, sizeof(descriptions[i])) != NULL;
            fclose(file);
        }
        if (!valid[i])
        {
            snprintf(descriptions[i], sizeof(descriptions[i]), "EMPTY");
        }
        for (char *c = descriptions[i]; *c; c++)
        {
            if ((unsigned char)*c < 32 || (unsigned char)*c > 126)
            {
                *c = ' ';
            }
        }
    }
}

static qboolean CanSave(void)
{
    if (!Com_ServerState() || cls.state != ca_active || Cvar_VariableValue("deathmatch") ||
        cl.frame.playerstate.stats[STAT_HEALTH] <= 0)
    {
        Com_Printf("Save requires a living player in a local non-deathmatch game.\n");
        return false;
    }
    return true;
}

static void Commit(void)
{
    if (saving)
    {
        if (!CanSave())
        {
            return;
        }
        if (!description[0])
        {
            snprintf(description, sizeof(description), "%s", cl.configstrings[CS_NAME]);
        }
        Cbuf_InsertText(va("save save%d \"%s\"\n", cursor, description));
    }
    else
    {
        Cbuf_InsertText(va("load save%d\n", cursor));
    }
    if (cursor > 0)
    {
        Cvar_SetValue("dn_save_slot", cursor);
    }
    M_ForceMenuOff();
}

static void BeginName(void)
{
    const char *name = valid[cursor] ? descriptions[cursor] : cl.configstrings[CS_NAME];
    size_t length = 0;
    /* Keep command delimiters out of the description passed to the engine. */
    for (; *name && length < DN_SAVE_NAME; name++)
    {
        unsigned char c = *name;
        if (isalnum(c) || c == ' ' || strchr("-.,!?:'", c))
        {
            description[length++] = toupper(c);
        }
    }
    description[length] = 0;
    editing = true;
}

static void SaveMenuDraw(void)
{
    DN_DrawUiTile(160, 19, 2457, 16, 0, 0);
    DN_MenuText(160, 24, saving ? "SAVE GAME" : "LOAD GAME", 1);
    if (confirming)
    {
        DN_MenuText(160, 80, saving ? "OVERWRITE THIS SAVE?" : "LOAD THIS GAME?", 1);
        char label[DN_SAVE_NAME + 1];
        snprintf(label, sizeof(label), "%s", descriptions[cursor]);
        DN_MenuText(160, 105, label, 1);
        DN_MenuText(105, 140, "YES", confirm_choice == 0);
        DN_MenuText(195, 140, "NO", confirm_choice == 1);
        DN_MenuCursor(confirm_choice ? 177 : 87, 133);
        for (int i = 0; i < 2; i++)
        {
            M_AddMouseItem((87 + i * 90) * viddef.width / 320, 125 * viddef.height / 200,
                           65 * viddef.width / 320, 20 * viddef.height / 200, &confirm_choice, i);
        }
        return;
    }
    int first = saving ? 1 : 0;
    for (int i = first; i <= DN_SAVE_SLOTS; i++)
    {
        int y = (saving ? 48 : 42) + (i - first) * 12;
        char label[32];
        const char *name = editing && i == cursor ? description : descriptions[i];
        if (!i)
        {
            snprintf(label, sizeof(label), "AUTO  %s", valid[i] ? "LEVEL START" : "EMPTY");
        }
        else
        {
            snprintf(label, sizeof(label), "%2d  %.*s%s", i, DN_SAVE_NAME, name,
                     editing && i == cursor && (cls.realtime / 250) % 2 == 0 ? ":" : "");
        }
        SmallText(100, y - 5, label);
        if (i == cursor)
        {
            DN_MenuCursor(81, y - 2);
        }
        if (!editing)
        {
            M_AddMouseItem(35 * viddef.width / 320, (y - 12) * viddef.height / 200,
                           270 * viddef.width / 320, 12 * viddef.height / 200, &cursor, i);
        }
    }
    SmallText(160, 186, editing ? "ENTER NAME - ENTER TO SAVE" : "ENTER SELECTS - ESC BACK");
}

static const char *SaveMenuKey(int key)
{
    if (confirming)
    {
        qboolean select = key == K_ENTER || key == K_KP_ENTER || key == K_MOUSE1;
        if (key == 'y' || key == 'Y' || (select && !confirm_choice))
        {
            confirming = false;
            if (saving && !quick)
            {
                BeginName();
            }
            else
            {
                Commit();
            }
        }
        else if (key == 'n' || key == 'N' || key == K_ESCAPE || (select && confirm_choice))
        {
            confirming = false;
            if (quick)
            {
                M_PopMenu();
            }
        }
        else if (key == K_LEFTARROW || key == K_RIGHTARROW || key == K_TAB || key == K_UPARROW ||
                 key == K_DOWNARROW)
        {
            confirm_choice ^= 1;
        }
        return "item15.wav";
    }
    if (editing)
    {
        size_t length = strlen(description);
        if (key == K_ESCAPE)
        {
            editing = false;
        }
        else if (key == K_ENTER || key == K_KP_ENTER)
        {
            Commit();
        }
        else if (key == K_BACKSPACE && length)
        {
            description[length - 1] = 0;
        }
        else if (key > 0 && key < 127 && length < DN_SAVE_NAME &&
                 (isalnum(key) || key == ' ' || strchr("-.,!?:'", key)))
        {
            description[length] = toupper(key);
            description[length + 1] = 0;
        }
        return NULL;
    }
    switch (key)
    {
    case K_ESCAPE:
        M_PopMenu();
        return "item15.wav";
    case K_UPARROW:
    case K_KP_UPARROW:
    case K_MWHEELUP:
        cursor--;
        break;
    case K_DOWNARROW:
    case K_KP_DOWNARROW:
    case K_MWHEELDOWN:
    case K_TAB:
        cursor++;
        break;
    case K_ENTER:
    case K_KP_ENTER:
    case K_MOUSE1:
        if (valid[cursor])
        {
            confirming = true;
            confirm_choice = 0;
        }
        else if (saving)
        {
            BeginName();
        }
        return "kickhit.wav";
    default:
        return NULL;
    }
    int first = saving ? 1 : 0;
    if (cursor < first)
    {
        cursor = DN_SAVE_SLOTS;
    }
    if (cursor > DN_SAVE_SLOTS)
    {
        cursor = first;
    }
    return "bulithit.wav";
}

static void OpenMenu(qboolean save, qboolean fast)
{
    if (save && !CanSave())
    {
        return;
    }
    saving = save;
    confirm_choice = 0;
    editing = confirming = quick = false;
    ReadSlots();
    int last = (int)Cvar_VariableValue("dn_save_slot");
    cursor = last >= 1 && last <= DN_SAVE_SLOTS ? last : (save ? 1 : 0);
    if (fast && last >= 1 && last <= DN_SAVE_SLOTS && valid[last])
    {
        cursor = last;
        quick = confirming = true;
        if (save)
        {
            BeginName();
            editing = false;
        }
    }
    M_PushMenu(SaveMenuDraw, SaveMenuKey);
}

void M_Menu_SaveGame_f(void)
{
    OpenMenu(true, false);
}

void M_Menu_LoadGame_f(void)
{
    OpenMenu(false, false);
}

static void QuickSave(void)
{
    OpenMenu(true, true);
}

static void QuickLoad(void)
{
    OpenMenu(false, true);
}

const char *DN_SaveMenuName(void)
{
    extern void (*m_drawfunc)(void);
    if (m_drawfunc != SaveMenuDraw)
    {
        return NULL;
    }
    return saving ? (editing      ? "save-name"
                     : confirming ? "save-confirm"
                                  : "save")
                  : (confirming ? "load-confirm" : "load");
}

void DN_InitSaveMenu(void)
{
    Cvar_Get("dn_save_slot", "-1", CVAR_ARCHIVE);
    Cmd_AddCommand("quicksave", QuickSave);
    Cmd_AddCommand("quickload", QuickLoad);
}
