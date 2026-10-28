/* Client prediction and presentation. Authoritative gameplay stays in dngame. */
#include "cgame.h"
#include "../qcommon/dntransport.h"
#include "../qcommon/dnrandom.h"
#include "../qcommon/dnplayermove.h"
#include "../qcommon/dnmath.h"

static dn_cgame_import_t cg;
static dn_cgame_export_t exports;

void Com_Printf(char *format, ...)
{
    char text[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    cg.Print("%s", text);
}

static int Init(void)
{
    dn_con_import_t imports = {cg.ReadFile, cg.Print, cg.Error};
    DN_ConConfigure(&imports);
    if (!DN_ConLoad())
    {
        return 0;
    }
    cg.Print("Duke cgame CON checksum %08x\n", DN_ConChecksum());
    size_t size = 0;
    void *tables = cg.ReadFile("sprites/tables.dat", &size);
    int valid = DN_MathInit(tables, size);
    free(tables);
    if (!valid)
    {
        cg.Error("Missing Build TABLES.DAT for player prediction");
        return 0;
    }
    return 1;
}

static void PredictMove(pmove_t *move, dn_prediction_t *state)
{
    dn_transport_query_t query = {};
    for (int axis = 0; axis < 3; axis++)
    {
        query.previous[axis] = move->s.origin[axis] / 8.f;
    }
    const dn_map_t *map = cg.CollisionMap();
    if (map && map->numsectors && move->s.pm_type != PM_SPECTATOR)
    {
        DN_PlayerMove(move, map, state);
    }
    else
    {
        DN_ResetPlayerMotion(state);
        cg.Pmove(move);
    }
    if (!map || !map->numsectors || move->s.pm_type == PM_SPECTATOR)
    {
        return;
    }
    query.has_previous = 1;
    for (int axis = 0; axis < 3; axis++)
    {
        query.point[axis] = move->s.origin[axis] / 8.f;
    }
    query.sector = state->sector = DN_FindSector(map, query.point, state->sector);
    query.hold = state->transport_hold;
    query.jetpack = state->jetpack_on;
    query.grounded = (move->s.pm_flags & PMF_ON_GROUND) != 0;
    query.jumping = move->s.velocity[2] > 0;
    query.upmove = move->cmd.upmove;
    query.eyeheight = move->viewheight;
    dn_transport_result_t result;
    if (!DN_QueryPlayerTransport(map, &query, &result))
    {
        return;
    }
    state->sector = map->sprites[result.destination].sectnum;
    DN_PushMove(map, result.point, move->mins, move->maxs, state->sector, MASK_PLAYERSOLID);
    for (int axis = 0; axis < 3; axis++)
    {
        move->s.origin[axis] = (int32_t)lroundf(result.point[axis] * 8);
    }
    move->s.pm_flags &= ~PMF_ON_GROUND;
    if (result.onfloor)
    {
        move->viewangles[YAW] = result.yaw;
        move->s.delta_angles[YAW] = ANGLE2SHORT(result.yaw) - move->cmd.angles[YAW];
        state->transport_hold = result.hold;
    }
}

static int SoundInfo(int id, dn_sound_t *sound)
{
    if (!sound || id < 0 || id >= 1024 || !dn_sounds[id].file[0])
    {
        return 0;
    }
    *sound = dn_sounds[id];
    return 1;
}

extern "C" __declspec(dllexport) dn_cgame_export_t *GetCGameAPI(const dn_cgame_import_t *imports)
{
    if (!imports || imports->version != DN_CGAME_API_VERSION || imports->size != sizeof(*imports) ||
        !imports->Print || !imports->Error || !imports->ReadFile || !imports->Pmove ||
        !imports->CollisionMap)
    {
        return NULL;
    }
    cg = *imports;
    exports.version = DN_CGAME_API_VERSION;
    exports.size = sizeof(exports);
    exports.Init = Init;
    exports.Shutdown = DN_ConFree;
    exports.ConChecksum = DN_ConChecksum;
    exports.PredictMove = PredictMove;
    exports.BuildWeaponView = DN_BuildWeaponView;
    exports.ExecuteCon = DN_ConExecute;
    exports.RandomNext = DN_RandomNext;
    exports.PredictAnimation = DN_PredictAnimation;
    exports.PredictWeapon = DN_PredictWeapon;
    exports.SoundInfo = SoundInfo;
    return &exports;
}
