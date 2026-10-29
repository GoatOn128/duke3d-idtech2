/* Module-private UI declarations: import binding and trap wrappers. */
#ifndef DN_UI_LOCAL_H
#define DN_UI_LOCAL_H
#include "ui.h"

void UI_BindImports(const dn_ui_import_t *imports);

void trap_Print(const char *fmt, ...);
void trap_Error(const char *fmt, ...);
int trap_Milliseconds(void);
void trap_Cvar_Register(const char *name, const char *value, uint32_t flags);
void trap_Cvar_Set(const char *name, const char *value);
void trap_Cvar_SetValue(const char *name, float value);
float trap_Cvar_VariableValue(const char *name);
void trap_Cvar_VariableStringBuffer(const char *name, char *buffer, uint32_t size);
int trap_Argc(void);
const char *trap_Argv(int index);
void trap_Cmd_ExecuteText(int when, const char *text);
void *trap_FS_ReadFile(const char *path, size_t *length);
int trap_Key_IsDown(int key);
void trap_Key_SetBinding(int key, const char *binding);
void trap_Key_GetBindingBuf(int key, char *buffer, uint32_t size);
void trap_Key_KeynumToStringBuf(int key, char *buffer, uint32_t size);
int trap_Key_GetOverstrikeMode(void);
void trap_Key_SetOverstrikeMode(int mode);
void trap_Key_ClearStates(void);
void trap_GetVideoMode(int *width, int *height);
int trap_GetClientState(void);
void *trap_R_RegisterShaderNoMip(const char *name);
void trap_R_GetPicSize(void *pic, int *width, int *height);
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, void *pic);
void trap_S_StartLocalSound(const char *name);
void trap_UpdateScreen(void);
void trap_DN_UiTile(float x, float y, int tile, int shade, int palette, int topLeft);
int trap_DN_MenuText(int x, int y, const char *text, int selected);
void trap_DN_MenuCursor(int x, int y);
void trap_DN_MenuBackground(void);
void trap_DN_MenuSound(int sound);
#endif
