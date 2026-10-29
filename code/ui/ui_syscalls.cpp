/* idTech 3-style trap wrappers over the UI module import table. */
#include "ui_local.h"
#include <stdarg.h>
#include <stdio.h>

static dn_ui_import_t ui;

void UI_BindImports(const dn_ui_import_t *imports)
{
    ui = *imports;
}

void trap_Print(const char *fmt, ...)
{
    char text[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    ui.Print("%s", text);
}

void trap_Error(const char *fmt, ...)
{
    char text[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    ui.Error("%s", text);
}

int trap_Milliseconds(void)
{
    return ui.Milliseconds();
}

void trap_Cvar_Register(const char *name, const char *value, uint32_t flags)
{
    ui.Cvar_Register(name, value, flags);
}

void trap_Cvar_Set(const char *name, const char *value)
{
    ui.Cvar_Set(name, value);
}

void trap_Cvar_SetValue(const char *name, float value)
{
    ui.Cvar_SetValue(name, value);
}

float trap_Cvar_VariableValue(const char *name)
{
    return ui.Cvar_Value(name);
}

void trap_Cvar_VariableStringBuffer(const char *name, char *buffer, uint32_t size)
{
    ui.Cvar_String(name, buffer, size);
}

int trap_Argc(void)
{
    return ui.Argc();
}

const char *trap_Argv(int index)
{
    return ui.Argv(index);
}

void trap_Cmd_ExecuteText(int when, const char *text)
{
    ui.ExecuteText(when, text);
}

void *trap_FS_ReadFile(const char *path, size_t *length)
{
    return ui.ReadFile(path, length);
}

int trap_Key_IsDown(int key)
{
    return ui.Key_IsDown(key);
}

void trap_Key_SetBinding(int key, const char *binding)
{
    ui.Key_SetBinding(key, binding);
}

void trap_Key_GetBindingBuf(int key, char *buffer, uint32_t size)
{
    ui.Key_GetBindingBuf(key, buffer, size);
}

void trap_Key_KeynumToStringBuf(int key, char *buffer, uint32_t size)
{
    ui.Key_KeynumToStringBuf(key, buffer, size);
}

int trap_Key_GetOverstrikeMode(void)
{
    return ui.GetOverstrikeMode();
}

void trap_Key_SetOverstrikeMode(int mode)
{
    ui.SetOverstrikeMode(mode);
}

void trap_Key_ClearStates(void)
{
    ui.Key_ClearStates();
}

void trap_GetVideoMode(int *width, int *height)
{
    ui.GetVideoMode(width, height);
}

int trap_GetClientState(void)
{
    return ui.GetConnState();
}

void *trap_R_RegisterShaderNoMip(const char *name)
{
    return ui.RegisterPic(name);
}

void trap_R_GetPicSize(void *pic, int *width, int *height)
{
    ui.GetPicSize(pic, width, height);
}

void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, void *pic)
{
    ui.DrawStretchPic(x, y, w, h, s1, t1, s2, t2, pic);
}

void trap_S_StartLocalSound(const char *name)
{
    ui.StartLocalSound(name);
}

void trap_UpdateScreen(void)
{
    ui.UpdateScreen();
}

void trap_DN_UiTile(float x, float y, int tile, int shade, int palette, int topLeft)
{
    ui.UiTile(x, y, tile, shade, palette, topLeft);
}

int trap_DN_MenuText(int x, int y, const char *text, int selected)
{
    return ui.MenuText(x, y, text, selected);
}

void trap_DN_MenuCursor(int x, int y)
{
    ui.MenuCursor(x, y);
}

void trap_DN_MenuBackground(void)
{
    ui.MenuBackground();
}

void trap_DN_MenuSound(int sound)
{
    ui.MenuSound(sound);
}
