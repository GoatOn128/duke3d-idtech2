/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "client.h"
#include "../qcommon/dnmap.h"

/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError(void)
{
    int frame;
    int delta[3];
    int i;
    int len;

    if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
    {
        return;
    }
    if (cl_paused && cl_paused->value)
    {
        VectorClear(cl.prediction_error);
        return; /* Paused commands were acknowledged without being simulated. */
    }

    // calculate the last usercmd_t we sent that the server has processed
    frame = cls.netchan.incoming_acknowledged;
    frame &= (CMD_BACKUP - 1);

    // compare what the server returned with what we had predicted it to be
    VectorSubtract(cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

    // save the prediction error for interpolation
    len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
    if (len > 640) // 80 world units
    {              // a teleport or something
        VectorClear(cl.prediction_error);
    }
    else
    {
        if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]))
        {
            Com_Printf("prediction miss on %i: %i\n", cl.frame.serverframe,
                       delta[0] + delta[1] + delta[2]);
        }

        VectorCopy(cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame]);

        // save for error itnerpolation
        for (i = 0; i < 3; i++)
        {
            cl.prediction_error[i] = delta[i] * 0.125;
        }
    }
}

/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr)
{
    int i;
    trace_t trace;
    int headnode;
    float *angles;
    entity_state_t *ent;
    int num;
    cmodel_t *cmodel;

    for (i = 0; i < cl.frame.num_entities; i++)
    {
        num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
        ent = &cl_parse_entities[num];

        if (!ent->solid)
        {
            continue;
        }
        if ((ent->renderfx & RF_DUKE_SPRITE) && !(((uint32_t)ent->skinnum >> 16) & 1))
        {
            continue; /* Build hitscan-only sprites do not stop player prediction. */
        }

        if (ent->number == cl.playernum + 1)
        {
            continue;
        }

        if (ent->solid == 31)
        { // special value for bmodel
            cmodel = cl.model_clip[ent->modelindex];
            if (!cmodel)
            {
                continue;
            }
            headnode = cmodel->headnode;
            angles = ent->angles;
        }
        else
        { // Exact sprite/player bounds received from gi.linkentity.
            headnode = CM_HeadnodeForBox(ent->mins, ent->maxs);
            angles = vec3_origin; // boxes don't rotate
        }

        if (tr->allsolid)
        {
            return;
        }

        if (DN_IsFloorSprite(ent))
        {
            trace = DN_TraceFloorSprite(ent, start, end, mins, maxs);
        }
        else
        {
            trace = CM_TransformedBoxTrace(start, end, mins, maxs, headnode, MASK_PLAYERSOLID,
                                           ent->origin, angles);
        }

        if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
        {
            trace.ent = (struct edict_s *)ent;
            if (tr->startsolid)
            {
                *tr = trace;
                tr->startsolid = true;
            }
            else
            {
                *tr = trace;
            }
        }
        else if (trace.startsolid)
        {
            tr->startsolid = true;
        }
    }
}

/*
================
CL_PMTrace
================
*/
trace_t CL_PMTrace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
    trace_t t;
    qboolean previous_query = CM_SetClientQuery(true);

    // check against world
    t = CM_BoxTrace(start, end, mins, maxs, 0, MASK_PLAYERSOLID);
    if (t.fraction < 1.0)
    {
        t.ent = (struct edict_s *)1;
    }

    // check all other solid models
    CL_ClipMoveToEntities(start, mins, maxs, end, &t);
    CM_SetClientQuery(previous_query);
    return t;
}

int CL_PMpointcontents(vec3_t point)
{
    int i;
    entity_state_t *ent;
    int num;
    cmodel_t *cmodel;
    int contents;
    qboolean previous_query = CM_SetClientQuery(true);

    contents = CM_PointContents(point, 0);

    for (i = 0; i < cl.frame.num_entities; i++)
    {
        num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
        ent = &cl_parse_entities[num];

        if (ent->solid != 31) // special value for bmodel
        {
            continue;
        }

        cmodel = cl.model_clip[ent->modelindex];
        if (!cmodel)
        {
            continue;
        }

        contents |= CM_TransformedPointContents(point, cmodel->headnode, ent->origin, ent->angles);
    }

    CM_SetClientQuery(previous_query);
    return contents;
}

/* Present the unfinished native tick without advancing command history. The
   copied move uses the same collision as replay, so the camera stops at walls.
   Corrections must continue comparing the discrete, acknowledged positions. */
static void CL_PredictNativeOrigin(const pmove_t *move, const dn_prediction_t *prediction)
{
    if (cl.frame.playerstate.stats[DN_STAT_FORMAT] != DN_PLAYER_FORMAT ||
        move->s.pm_type != PM_NORMAL || !prediction->motion_initialized ||
        prediction->motion_time <= 0)
    {
        return;
    }

    pmove_t next = *move;
    dn_prediction_t state = *prediction;
    next.cmd = cl.cmd;
    next.cmd.msec = (1000 - state.motion_time + 29) / 30;
    CL_CGamePredict(&next, &state);

    vec3_t point = {state.motion_x / 16.f, -state.motion_y / 16.f, -state.motion_z / 256.f - 12};
    for (int axis = 0; axis < 3; axis++)
    {
        /* Transport changes the pmove origin after the native move. Wait for
           the real tick rather than interpolating through the intervening map. */
        if ((int32_t)lroundf(point[axis] * 8) != next.s.origin[axis] ||
            abs(next.s.origin[axis] - move->s.origin[axis]) > 640 ||
            next.s.delta_angles[axis] != move->s.delta_angles[axis])
        {
            return;
        }
    }
    float fraction = prediction->motion_time / 1000.f;
    for (int axis = 0; axis < 3; axis++)
    {
        cl.predicted_origin[axis] += fraction * (point[axis] - cl.predicted_origin[axis]);
    }
}

/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CL_PredictMovement(void)
{
    int ack, current;
    int frame;
    int oldframe;
    usercmd_t *cmd;
    pmove_t pm;
    int i;
    int step;
    int oldz;

    if (cls.state != ca_active)
    {
        return;
    }

    if (cl_paused->value)
    {
        for (i = 0; i < 3; i++)
        {
            cl.predicted_origin[i] = cl.frame.playerstate.pmove.origin[i] / 8.f;
            cl.predicted_angles[i] = cl.frame.playerstate.viewangles[i];
        }
        cl.predicted_sway = cl.frame.playerstate.prediction.motion_sway;
        cl.predicted_step = 0;
        VectorClear(cl.prediction_error);
        return;
    }

    if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
    { // just set angles
        for (i = 0; i < 3; i++)
        {
            cl.predicted_angles[i] =
                cl.viewangles[i] + SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
        }
        return;
    }

    ack = cls.netchan.incoming_acknowledged;
    current = cls.netchan.outgoing_sequence;

    // if we are too far out of date, just freeze
    if (current - ack >= CMD_BACKUP)
    {
        if (cl_showmiss->value)
        {
            Com_Printf("exceeded CMD_BACKUP\n");
        }
        return;
    }

    // copy current state to pmove
    memset(&pm, 0, sizeof(pm));
    pm.trace = CL_PMTrace;
    pm.pointcontents = CL_PMpointcontents;

    pm_airaccelerate = atof(cl.configstrings[CS_AIRACCEL]);

    pm.s = cl.frame.playerstate.pmove;
    for (i = 0; i < 3; i++)
    {
        pm.viewangles[i] = cl.viewangles[i] + SHORT2ANGLE(pm.s.delta_angles[i]);
    }
    dn_prediction_t prediction = cl.frame.playerstate.prediction;

    //	SCR_DebugGraph (current - ack - 1, 0);

    frame = 0;

    // run frames
    while (++ack < current)
    {
        frame = ack & (CMD_BACKUP - 1);
        cmd = &cl.cmds[frame];

        pm.cmd = *cmd;
        CL_CGamePredict(&pm, &prediction);

        // save for debug checking
        VectorCopy(pm.s.origin, cl.predicted_origins[frame]);
    }

    oldframe = (ack - 2) & (CMD_BACKUP - 1);
    oldz = cl.predicted_origins[oldframe][2];
    step = pm.s.origin[2] - oldz;
    if (cl.frame.playerstate.stats[DN_STAT_FORMAT] != DN_PLAYER_FORMAT && step > 63 && step < 160 &&
        (pm.s.pm_flags & PMF_ON_GROUND))
    {
        cl.predicted_step = step * 0.125;
        cl.predicted_step_time = cls.realtime - cls.frametime * 500;
    }
    if (cl.frame.playerstate.stats[DN_STAT_FORMAT] == DN_PLAYER_FORMAT)
    {
        cl.predicted_step = 0; /* Native slopes already smooth their floor height. */
    }

    // copy results out for rendering
    cl.predicted_origin[0] = pm.s.origin[0] * 0.125;
    cl.predicted_origin[1] = pm.s.origin[1] * 0.125;
    cl.predicted_origin[2] = pm.s.origin[2] * 0.125;
    CL_PredictNativeOrigin(&pm, &prediction);

    VectorCopy(pm.viewangles, cl.predicted_angles);
    cl.predicted_sway = prediction.motion_sway;
}
