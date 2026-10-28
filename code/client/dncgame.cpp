/* Loading and engine services for the replaceable client-game DLL. */
#include "client.h"
#include "../cgame/cgame.h"
#include <windows.h>

static HMODULE library;
static dn_cgame_export_t *cgame;
const dn_map_t *CM_DukeClientMap(void);

static void Error(char *format, ...)
{
    char text[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    Com_Error(ERR_DROP, "%s", text);
}

static void *ReadAsset(const char *path, size_t *length)
{
    void *data = NULL;
    int size = FS_LoadFile((char *)path, &data);
    if (size < 0 || !data)
    {
        return NULL;
    }
    char *copy = (char *)(malloc((size_t)size + 1));
    if (!copy)
    {
        FS_FreeFile(data);
        return NULL;
    }
    memcpy(copy, data, size);
    copy[size] = 0;
    *length = size;
    FS_FreeFile(data);
    return copy;
}

void CL_UnloadCGame(void)
{
    if (cgame)
    {
        cgame->Shutdown();
        cgame = NULL;
    }
    if (library)
    {
        FreeLibrary(library);
        library = NULL;
    }
}

dn_cgame_export_t *CL_LoadCGame(void)
{
    if (cgame)
    {
        return cgame;
    }
    char name[MAX_OSPATH];
    for (char *path = FS_NextPath(NULL); path; path = FS_NextPath(path))
    {
        Com_sprintf(name, sizeof(name), "%s/cgamex86.dll", path);
        library = LoadLibraryA(name);
        if (library)
        {
            break;
        }
    }
    if (!library)
    {
        Com_Error(ERR_DROP, "Cannot load basedn/cgamex86.dll");
    }
    dn_get_cgame_api_t get_api = (dn_get_cgame_api_t)GetProcAddress(library, "GetCGameAPI");
    dn_cgame_import_t imports = {
        DN_CGAME_API_VERSION, sizeof(imports), Com_Printf, Error, ReadAsset, Pmove,
        CM_DukeClientMap};
    cgame = get_api ? get_api(&imports) : NULL;
    if (!cgame || cgame->version != DN_CGAME_API_VERSION || cgame->size != sizeof(*cgame))
    {
        cgame = NULL;
        CL_UnloadCGame();
        Com_Error(ERR_DROP, "Invalid cgame API");
    }
    if (!cgame->Init())
    {
        CL_UnloadCGame();
        Com_Error(ERR_DROP, "Client CON compilation failed");
    }
    Com_Printf("Duke client game library: %s\n", name);
    return cgame;
}

int CL_CGameSoundInfo(int id, dn_sound_t *sound)
{
    return CL_LoadCGame()->SoundInfo(id, sound);
}

static void CheckProgram(const dn_cgame_export_t *module, const dn_prediction_t *state)
{
    if (state->version &&
        (state->version != DN_PREDICTION_VERSION || state->program != module->ConChecksum()))
    {
        Com_Error(ERR_DROP, "Client/server CON program mismatch");
    }
}

void CL_CGamePredict(pmove_t *move, dn_prediction_t *state)
{
    dn_cgame_export_t *module = CL_LoadCGame();
    CheckProgram(module, state);
    module->PredictMove(move, state);
}

void CL_CGameWeaponView(const dn_weapon_view_t *view, dn_weapon_sprites_t *sprites)
{
    dn_cgame_export_t *module = CL_LoadCGame();
    CheckProgram(module, &cl.frame.playerstate.prediction);
    dn_weapon_view_t predicted = *view;
    if (Cvar_Get("cl_dn_predictanimation", "1", CVAR_ARCHIVE)->value &&
        !(cl_paused && cl_paused->value) &&
        cl.frame.playerstate.stats[DN_STAT_FORMAT] == DN_PLAYER_FORMAT &&
        cl.frame.playerstate.stats[DN_STAT_WEAPON] == view->weapon)
    {
        int elapsed = cls.realtime - cl.frame.receivedtime;
        elapsed = elapsed < 0 ? 0 : elapsed > 200 ? 200 : elapsed;
        dn_weapon_view_t source = *view;
        source.phase = cl.frame.playerstate.stats[DN_STAT_WEAPON_FRAME];
        module->PredictWeapon(&source, cl.frame.playerstate.stats[STAT_AMMO],
                              !!(cl.cmd.buttons & BUTTON_ATTACK), elapsed * 30 / 1000, &predicted);
    }
    module->BuildWeaponView(&predicted, sprites);
}

int CL_CGameAnimationFrame(const entity_state_t *state, int elapsed_ms)
{
    if (!Cvar_Get("cl_dn_predictanimation", "1", CVAR_ARCHIVE)->value ||
        (cl_paused && cl_paused->value) || elapsed_ms <= 0 ||
        !(state->animation.flags & DN_ANIMATION_RUNNING))
    {
        return state->frame;
    }
    if (elapsed_ms > 200)
    {
        elapsed_ms = 200;
    }
    uint32_t target = state->animation.tick + elapsed_ms * 30 / 1000;
    dn_cgame_export_t *module = CL_LoadCGame();
    CheckProgram(module, &cl.frame.playerstate.prediction);
    return module->PredictAnimation(state, target);
}
