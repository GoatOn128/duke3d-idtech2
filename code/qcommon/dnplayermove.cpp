/* PLAYER.cpp processinput/getinput movement, in Build's integer units at 30 Hz.
   Coordinates and momentum stay in the acknowledged prediction baseline. */
#include "dnplayermove.h"
#include "dnmath.h"
#include "dncon.h"

void DN_ResetPlayerMotion(dn_prediction_t *state)
{
    state->motion_initialized = state->motion_time = 0;
    state->motion_vx = state->motion_vy = state->motion_vz = 0;
    state->motion_jump = state->motion_toggle = state->motion_fall = 0;
    state->motion_landing = 0;
    state->motion_sway = 1024;
    state->motion_bob = state->motion_speed = 0;
}

static void Position(const dn_prediction_t *state, vec3_t point)
{
    VectorSet(point, state->motion_x / 16.f, -state->motion_y / 16.f,
              -state->motion_z / 256.f - 12);
}

static void Heights(pmove_t *move, const dn_map_t *map, dn_prediction_t *state, int *floor,
                    int *ceiling)
{
    vec3_t start, end, mins = {-10.25f, -10.25f, 0}, maxs = {10.25f, 10.25f, 0};
    Position(state, start);
    start[2] += 12;
    state->sector = DN_FindSector(map, start, state->sector);
    if (state->sector < 0)
    {
        *floor = state->motion_z + 40 * 256;
        *ceiling = state->motion_z - 64 * 256;
        return;
    }
    float native_floor, native_ceiling;
    DN_SectorZRange(map, start, state->sector, 163, &native_floor, &native_ceiling);
    *floor = (int)lroundf(-256 * native_floor);
    *ceiling = (int)lroundf(-256 * native_ceiling);
    VectorCopy(start, end);
    end[2] = -*floor / 256.f - 1;
    trace_t trace = move->trace(start, mins, maxs, end);
    if (!trace.startsolid && trace.fraction < 1 && trace.ent && trace.ent != (struct edict_s *)1 &&
        trace.plane.normal[2] > .5f)
    {
        float surface = (trace.plane.dist - trace.plane.normal[0] * start[0] -
                         trace.plane.normal[1] * start[1]) /
                        trace.plane.normal[2];
        if (fabsf(surface - trace.endpos[2]) > .25f)
        {
            surface = trace.endpos[2];
        }
        if (fabsf(surface + *floor / 256.f) > .125f)
        {
            *floor = (int)lroundf(-surface * 256);
        }
        const entity_state_t *sprite = (const entity_state_t *)trace.ent;
        int tile = sprite->animation.tile ? sprite->animation.tile : sprite->frame;
        int repeat = sprite->skinnum & 255;
        if ((sprite->renderfx & RF_DUKE_SPRITE) && DN_IsEnemy(tile) && repeat > 24 &&
            fabsf(start[2] - 40 - sprite->origin[2]) < 84)
        {
            int angle = DN_GetAngle((int)lroundf(sprite->origin[0] * 16) - state->motion_x,
                                    (int)lroundf(-sprite->origin[1] * 16) - state->motion_y);
            state->motion_vx -= DN_Sin(angle + 512) * 16;
            state->motion_vy -= DN_Sin(angle) * 16;
        }
        move->groundentity = trace.ent;
    }
    end[2] = -*ceiling / 256.f + 1;
    trace = move->trace(start, mins, maxs, end);
    if (!trace.startsolid && trace.fraction < 1 && trace.ent && trace.ent != (struct edict_s *)1 &&
        trace.plane.normal[2] < -.5f)
    {
        float surface = (trace.plane.dist - trace.plane.normal[0] * start[0] -
                         trace.plane.normal[1] * start[1]) /
                        trace.plane.normal[2];
        if (fabsf(surface - trace.endpos[2]) > .25f)
        {
            surface = trace.endpos[2];
        }
        if (fabsf(surface + *ceiling / 256.f) > .125f)
        {
            *ceiling = (int)lroundf(-surface * 256);
        }
    }
}

static void Vertical(pmove_t *move, const dn_map_t *map, dn_prediction_t *state)
{
    int floor, ceiling;
    Heights(move, map, state, &floor, &ceiling);
    int lotag = state->sector >= 0 ? map->sectors[state->sector].lotag : 0;
    int jump = move->cmd.upmove > 10, duck = move->cmd.upmove < -10;
    int *z = &state->motion_z, *velocity = &state->motion_vz;
    if (state->jetpack_on || lotag == 2)
    {
        state->motion_jump = state->motion_fall = 0;
        move->s.pm_flags &= ~PMF_ON_GROUND;
        if (state->jetpack_on)
        {
            *velocity = 0;
            *z += duck ? 2048 : jump ? -2048 : 0;
            if (*z > floor - 32 * 256)
            {
                *z += (floor - 32 * 256 - *z) >> 1;
            }
            if (*z < ceiling + 18 * 256)
            {
                *z = ceiling + 18 * 256;
            }
        }
        else
        {
            if (jump)
            {
                if (*velocity > 0)
                {
                    *velocity = 0;
                }
                *velocity = DN_Max(-1536, *velocity - 348);
            }
            else if (duck)
            {
                if (*velocity < 0)
                {
                    *velocity = 0;
                }
                *velocity = DN_Min(1536, *velocity + 348);
            }
            else if (*velocity < 0)
            {
                *velocity = DN_Min(0, *velocity + 256);
            }
            else if (*velocity > 0)
            {
                *velocity = DN_Max(0, *velocity - 256);
            }
            if (*velocity > 2048)
            {
                *velocity >>= 1;
            }
            *z += *velocity;
            if (*z > floor - 15 * 256)
            {
                *z += (floor - 15 * 256 - *z) >> 1;
            }
            if (*z < ceiling + 4 * 256)
            {
                *z = ceiling + 4 * 256;
                *velocity = 0;
            }
        }
        return;
    }
    int height = lotag == 1 ? 34 : 40;
    int target = floor - height * 256;
    if (*z < target)
    {
        if (!jump && !duck && (move->s.pm_flags & PMF_ON_GROUND) && state->sector >= 0 &&
            (map->sectors[state->sector].floorstat & 2) && *z >= target - 16 * 256)
        {
            *z = target;
        }
        else
        {
            move->s.pm_flags &= ~PMF_ON_GROUND;
            *velocity = DN_Min(6144, *velocity + DN_ConSetting("GRAVITATIONALCONSTANT", 176) + 80);
            if (*velocity > 2400 && state->motion_fall < 255)
            {
                ++state->motion_fall;
            }
            if (*z + *velocity >= target && lotag != 1)
            {
                state->motion_landing = DN_Max(state->motion_landing, state->motion_fall);
            }
        }
    }
    else
    {
        state->motion_fall = 0;
        move->s.pm_flags |= PMF_ON_GROUND;
        if (height == 40)
        {
            int correction = (target - *z) >> 1;
            if (abs(correction) < 256)
            {
                correction = 0;
            }
            *z += correction;
            *velocity = DN_Max(0, *velocity - 768);
        }
        else if (!state->motion_jump)
        {
            *z += (floor - height * 128 - *z) >> 1;
            if (*z > floor - 16 * 256)
            {
                *z = floor - 16 * 256;
                *velocity >>= 1;
            }
        }
        if (duck)
        {
            *z += 2816;
        }
        if (!jump && state->motion_toggle)
        {
            state->motion_toggle = 0;
        }
        else if (jump && !state->motion_toggle && !state->motion_jump && floor - ceiling > 56 * 256)
        {
            state->motion_jump = state->motion_toggle = 1;
        }
    }
    if (state->motion_jump)
    {
        if (!jump)
        {
            state->motion_toggle = 0;
        }
        if (state->motion_jump < 1280)
        {
            if (lotag == 1 && state->motion_jump > 768)
            {
                state->motion_jump = 0;
                *velocity = -512;
            }
            else
            {
                *velocity -= DN_Sin(2048 - 128 + state->motion_jump) / 12;
                state->motion_jump += 180;
                move->s.pm_flags &= ~PMF_ON_GROUND;
            }
        }
        else
        {
            state->motion_jump = 0;
            *velocity = 0;
        }
    }
    *z += *velocity;
    if (*z < ceiling + 4 * 256)
    {
        state->motion_jump = 0;
        if (*velocity < 0)
        {
            state->motion_vx = state->motion_vy = 0;
        }
        *velocity = 128;
        *z = ceiling + 4 * 256;
    }
}

static void Horizontal(pmove_t *move, const dn_map_t *map, dn_prediction_t *state)
{
    int angle = (int)lroundf(-move->viewangles[YAW] * 2048 / 360) & 2047;
    int forward = DN_Max(-90, DN_Min(90, move->cmd.forwardmove / 5));
    int side = DN_Max(-90, DN_Min(90, -move->cmd.sidemove / 5));
    int input_x = ((forward * DN_Sin(angle + 512)) >> 9) + ((side * DN_Sin(angle)) >> 9);
    int input_y = ((forward * DN_Sin(angle)) >> 9) + ((side * DN_Sin(angle - 512)) >> 9);
    int running_friction = DN_ConSetting("RUNNINGSPEED", 53200);
    int friction = running_friction;
    int lotag = state->sector >= 0 ? map->sectors[state->sector].lotag : 0;
    int crouch = move->cmd.upmove < -10;
    int multiplier =
        state->motion_steroids > 0 && state->motion_steroids < 400 && !state->jetpack_on ? 512
                                                                                         : 256;
    if ((move->s.pm_flags & PMF_ON_GROUND) && crouch)
    {
        friction -= 0x2000;
    }
    else if (lotag == 2)
    {
        friction -= 0x1400;
    }
    state->motion_vx =
        (int32_t)(((int64_t)state->motion_vx + input_x * multiplier) * friction >> 16);
    state->motion_vy =
        (int32_t)(((int64_t)state->motion_vy + input_y * multiplier) * friction >> 16);
    if (abs(state->motion_vx) < 2048 && abs(state->motion_vy) < 2048)
    {
        state->motion_vx = state->motion_vy = 0;
    }
    if (state->xrepeat && state->xrepeat < 32)
    {
        int scale = running_friction - (running_friction >> 1) + (running_friction >> 2);
        state->motion_vx = (int32_t)((int64_t)state->motion_vx * scale >> 16);
        state->motion_vy = (int32_t)((int64_t)state->motion_vy * scale >> 16);
    }
    vec3_t point, end;
    Position(state, point);
    float floor =
        state->sector >= 0 ? DN_PlaneZ(map, state->sector, 0, point[0], point[1]) : point[2] - 28;
    /* Build clipmove uses a 20-unit floor clearance, allowing its native steps.
       Crouching lowers the eye; the collision feet must follow that change. */
    vec3_t mins = {-10.25f, -10.25f, -fmaxf(0, fminf(20, point[2] + 12 - floor - .125f)) + 12};
    vec3_t maxs = {10.25f, 10.25f, 16};
    vec3_t displacement = {(state->motion_vx >> 14) / 16.f, -(state->motion_vy >> 14) / 16.f, 0};
    if (state->sector >= 0)
    {
        /* Build's slope clearance is measured at the eye, not across a standing
           box. Remove the box's extra slope support to avoid allsolid sweeps at
           valid positions. Include nearby portals and this tick's plane change. */
        const dn_sector_t *sector = &map->sectors[state->sector];
        float top = maxs[2], bottom = mins[2];
        for (int index = -1; index < sector->wallnum; index++)
        {
            int next = state->sector;
            if (index >= 0)
            {
                const dn_wall_t *wall = &map->walls[sector->wallptr + index];
                if (wall->nextsector < 0 || (wall->cstat & 1))
                {
                    continue;
                }
                const dn_wall_t *endwall = &map->walls[wall->point2];
                float dx = (endwall->x - (float)wall->x) / 16;
                float dy = -(endwall->y - (float)wall->y) / 16;
                float x = point[0] - wall->x / 16.f, y = point[1] + wall->y / 16.f;
                float length2 = dx * dx + dy * dy;
                float along = length2 > 0 ? fmaxf(0, fminf(1, (x * dx + y * dy) / length2)) : 0;
                float reach = hypotf(maxs[0], maxs[1]) + VectorLength(displacement) + .125f;
                if (hypotf(x - dx * along, y - dy * along) > reach)
                {
                    continue;
                }
                next = wall->nextsector;
            }
            for (int ceiling = 0; ceiling < 2; ceiling++)
            {
                vec3_t normal;
                DN_PlaneNormal(map, next, ceiling, normal);
                float support =
                    (fabsf(normal[0]) * maxs[0] + fabsf(normal[1]) * maxs[1]) / fabsf(normal[2]);
                if (support == 0)
                {
                    continue;
                }
                float change = DotProduct(normal, displacement) / normal[2];
                float approach = fmaxf(0, ceiling ? change : -change);
                float height = DN_PlaneZ(map, next, ceiling, point[0], point[1]) - point[2];
                if (fabsf(height - (ceiling ? maxs[2] : mins[2])) <= support + approach + .125f)
                {
                    if (ceiling)
                    {
                        top = fminf(top, height - support - approach);
                    }
                    else
                    {
                        bottom = fmaxf(bottom, height + support + approach);
                    }
                }
            }
        }
        mins[2] = fminf(bottom, top);
        maxs[2] = fmaxf(bottom, top);
    }
    for (int slide = 0; slide < 3; slide++)
    {
        VectorAdd(point, displacement, end);
        trace_t trace = move->trace(point, mins, maxs, end);
        if (trace.allsolid)
        {
            break;
        }
        VectorCopy(trace.endpos, point);
        if (trace.fraction == 1)
        {
            break;
        }
        VectorScale(displacement, 1 - trace.fraction, displacement);
        float into = DotProduct(displacement, trace.plane.normal);
        VectorMA(displacement, -into, trace.plane.normal, displacement);
        if (trace.ent && move->numtouch < MAXTOUCH)
        {
            move->touchents[move->numtouch++] = trace.ent;
        }
    }
    DN_PushMove(map, point, mins, maxs, state->sector, MASK_PLAYERSOLID);
    state->motion_x = (int32_t)lroundf(point[0] * 16);
    state->motion_y = (int32_t)lroundf(-point[1] * 16);
    state->sector = DN_FindSector(map, point, state->sector);
    if (crouch)
    {
        move->s.pm_flags |= PMF_DUCKED;
    }
    else
    {
        move->s.pm_flags &= ~PMF_DUCKED;
    }
}

static void WeaponSway(pmove_t *move, dn_prediction_t *state)
{
    if (state->motion_speed < 32 || !(move->s.pm_flags & PMF_ON_GROUND) ||
        state->motion_bob == 1024)
    {
        int phase = state->motion_sway & 2047;
        state->motion_sway += phase > 1120 ? -96 : phase < 928 ? 96 : 0;
        if (phase >= 928 && phase <= 1120)
        {
            state->motion_sway = 1024;
        }
    }
    else
    {
        state->motion_sway = state->motion_bob;
    }
    int64_t dx = (int64_t)state->motion_x - state->motion_bobx;
    int64_t dy = (int64_t)state->motion_y - state->motion_boby;
    state->motion_speed = (int)sqrt((double)(dx * dx + dy * dy));
    if (move->s.pm_flags & PMF_ON_GROUND)
    {
        /* Half-speed sine indexing has a 4096-unit period. Wrapping at 2048
           makes the weapon jump sideways halfway through a walk cycle. */
        state->motion_bob =
            (int32_t)(((uint32_t)state->motion_bob + (state->motion_speed >> 1)) & INT32_MAX);
    }
    state->motion_bobx = state->motion_x;
    state->motion_boby = state->motion_y;
}

void DN_PlayerMove(pmove_t *move, const dn_map_t *map, dn_prediction_t *state)
{
    vec3_t point;
    Position(state, point);
    int changed = !state->motion_initialized;
    for (int axis = 0; axis < 3; axis++)
    {
        changed |= (int32_t)lroundf(point[axis] * 8) != move->s.origin[axis];
        move->viewangles[axis] =
            SHORT2ANGLE((short)(move->cmd.angles[axis] + move->s.delta_angles[axis]));
    }
    if (move->viewangles[PITCH] > 180)
    {
        move->viewangles[PITCH] -= 360;
    }
    move->viewangles[PITCH] = fmaxf(-89, fminf(89, move->viewangles[PITCH]));
    if (changed)
    {
        state->motion_x = move->s.origin[0] * 2;
        state->motion_y = -move->s.origin[1] * 2;
        state->motion_z = -move->s.origin[2] * 32 - 12 * 256;
        state->motion_initialized = 1;
        state->motion_bobx = state->motion_x;
        state->motion_boby = state->motion_y;
        state->motion_speed = 0;
        if (!state->motion_sway)
        {
            state->motion_sway = 1024;
        }
    }
    state->motion_landing = 0;
    move->numtouch = 0;
    if (move->s.pm_type == PM_NORMAL)
    {
        state->motion_time += move->cmd.msec * 30;
        while (state->motion_time >= 1000)
        {
            state->motion_time -= 1000;
            WeaponSway(move, state);
            Vertical(move, map, state);
            Horizontal(move, map, state);
        }
    }
    Position(state, point);
    for (int axis = 0; axis < 3; axis++)
    {
        move->s.origin[axis] = (int32_t)lroundf(point[axis] * 8);
    }
    move->s.velocity[0] = (int32_t)((int64_t)state->motion_vx * 240 / 262144);
    move->s.velocity[1] = (int32_t)(-(int64_t)state->motion_vy * 240 / 262144);
    move->s.velocity[2] = -state->motion_vz * 240 / 256;
    move->viewheight = 12;
    VectorSet(move->mins, -10.25f, -10.25f, (move->s.pm_flags & PMF_DUCKED) ? -6 : -28);
    VectorSet(move->maxs, 10.25f, 10.25f, 16);
    move->waterlevel = state->sector >= 0 && map->sectors[state->sector].lotag == 2 ? 3 : 0;
    move->watertype = move->waterlevel ? CONTENTS_WATER : 0;
}
