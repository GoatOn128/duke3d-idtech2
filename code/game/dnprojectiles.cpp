/* Authoritative projectile motion, remote charges and tripwire collision. */
#include "dnlocal.h"
#include "dncon.h"

static void Explode(edict_t *projectile, edict_t *direct)
{
    int radius =
        DN_ConSetting(projectile->projectile == DN_PROJECTILE_PIPEBOMB   ? "PIPEBOMBRADIUS"
                      : projectile->projectile == DN_PROJECTILE_TRIPBOMB ? "TRIPBOMBBLASTRADIUS"
                                                                         : "RPGBLASTRADIUS",
                      1780);
    edict_t *owner = projectile->owner ? projectile->owner : projectile;
    if (direct && direct != dn_edicts)
    {
        DN_Damage(direct, owner, projectile->damage, projectile->tile);
    }
    int damage[4] = {projectile->damage >> 2, projectile->damage >> 1,
                     projectile->damage - (projectile->damage >> 2), projectile->damage};
    DN_RadiusDamage(projectile, radius, damage, direct);
    edict_t *effect = DN_SpawnSprite(projectile, DN_ConSymbol("EXPLOSION2", 1890));
    effect->cstat = 0;
    DN_PlaySound(effect, DN_ConSymbol("PIPEBOMB_EXPLODE", 14), 0, 0);
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        if (dn_edicts[i].inuse && dn_edicts[i].owner == projectile &&
            dn_edicts[i].projectile == DN_PROJECTILE_BEAM)
        {
            DN_Remove(&dn_edicts[i]);
        }
    }
    DN_Remove(projectile);
}

void DN_DetonatePipebombs(edict_t *owner)
{
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        edict_t *projectile = &dn_edicts[i];
        if (projectile->inuse && projectile->owner == owner &&
            projectile->projectile == DN_PROJECTILE_PIPEBOMB)
        {
            projectile->firetime = dn_time;
        }
    }
}

int DN_PlaceTripbomb(edict_t *player)
{
    vec3_t eye, forward, end;
    VectorCopy(player->s.origin, eye);
    eye[2] += player->client ? player->client->ps.viewoffset[2] : 20;
    AngleVectors(player->client ? player->client->ps.viewangles : player->s.angles, forward, NULL,
                 NULL);
    VectorMA(eye, 290.f / 16, forward, end);
    trace_t trace = gi.trace(eye, vec3_origin, vec3_origin, end, player, MASK_SHOT);
    if (trace.fraction == 1 || trace.startsolid || fabsf(trace.plane.normal[2]) > .5f ||
        (trace.ent && trace.ent != dn_edicts))
    {
        return 0;
    }
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        edict_t *other = &dn_edicts[i];
        vec3_t delta;
        VectorSubtract(trace.endpos, other->s.origin, delta);
        if (other->inuse && other->projectile == DN_PROJECTILE_TRIPBOMB &&
            VectorLength(delta) < 18.125f)
        {
            return 0;
        }
    }
    edict_t *mine = DN_SpawnSprite(player, DN_ConSymbol("TRIPBOMB", 2566));
    mine->projectile = DN_PROJECTILE_TRIPBOMB;
    mine->actor = -1;
    mine->cstat = 16;
    mine->xrepeat = 4;
    mine->yrepeat = 5;
    mine->damage = DN_ConSymbol("TRIPBOMB_STRENGTH", 100);
    mine->firetime = 0;
    mine->count = 0;
    VectorMA(trace.endpos, .125f, trace.plane.normal, mine->s.origin);
    VectorCopy(trace.plane.normal, mine->velocity);
    mine->s.angles[1] = atan2f(mine->velocity[1], mine->velocity[0]) * 180 / (float)M_PI;
    DN_Link(mine);
    return 1;
}

static void TripwireTick(edict_t *mine)
{
    vec3_t end, delta;
    if (mine->count < 45)
    {
        return;
    }
    VectorMA(mine->s.origin, 1024, mine->velocity, end);
    trace_t trace = gi.trace(mine->s.origin, vec3_origin, vec3_origin, end, mine, MASK_SHOT);
    if (mine->count == 45)
    {
        VectorCopy(trace.endpos, mine->spawnorigin);
        edict_t *beam = DN_NewEntity();
        beam->owner = mine;
        beam->projectile = DN_PROJECTILE_BEAM;
        beam->s.renderfx = RF_BEAM;
        beam->s.effects = 0;
        beam->s.frame = 1;
        beam->s.skinnum = 242;
        beam->s.modelindex = dn_sprite_model;
        VectorCopy(mine->s.origin, beam->s.origin);
        VectorCopy(trace.endpos, beam->s.old_origin);
        beam->linkcount = 1; /* old_origin is the laser endpoint, not a previous position. */
        DN_Link(beam);
        DN_PlaySound(mine, DN_ConSymbol("LASERTRIP_ARMING", 176), 0, 0);
        return;
    }
    VectorSubtract(trace.endpos, mine->spawnorigin, delta);
    if ((trace.ent && trace.ent != dn_edicts) || VectorLength(delta) > .5f)
    {
        Explode(mine, NULL);
    }
}

void DN_ProjectileTick(edict_t *projectile)
{
    vec3_t end;
    if (projectile->projectile == DN_PROJECTILE_FIRELASER ||
        projectile->projectile == DN_PROJECTILE_LASERTRAIL)
    {
        DN_LaserTick(projectile);
        return;
    }
    if (projectile->projectile == DN_PROJECTILE_BEAM)
    {
        if (!projectile->owner || !projectile->owner->inuse ||
            projectile->owner->projectile != DN_PROJECTILE_TRIPBOMB)
        {
            DN_Remove(projectile);
        }
        return;
    }
    ++projectile->count;
    int charge = projectile->projectile == DN_PROJECTILE_PIPEBOMB ||
                 projectile->projectile == DN_PROJECTILE_TRIPBOMB;
    if (charge && projectile->firetime > 0 && projectile->firetime <= dn_time)
    {
        Explode(projectile, NULL);
        return;
    }
    if (projectile->projectile == DN_PROJECTILE_TRIPBOMB)
    {
        TripwireTick(projectile);
        return;
    }
    if (projectile->projectile == DN_PROJECTILE_PIPEBOMB ||
        projectile->projectile == DN_PROJECTILE_MORTAR)
    {
        projectile->velocity[2] -= 800.f / DN_SIMULATION_HZ;
    }
    VectorMA(projectile->s.origin, 1.f / DN_SIMULATION_HZ, projectile->velocity, end);
    trace_t trace =
        gi.trace(projectile->s.origin, vec3_origin, vec3_origin, end, projectile->owner, MASK_SHOT);
    DN_DamageWall(projectile->owner, projectile->s.origin, end, &trace);
    VectorCopy(trace.endpos, projectile->s.origin);
    if (trace.fraction < 1)
    {
        int freeze = projectile->projectile == DN_PROJECTILE_FREEZE;
        if (projectile->projectile == DN_PROJECTILE_PIPEBOMB ||
            (freeze && (!trace.ent || trace.ent == dn_edicts) &&
             projectile->inventory < DN_ConSetting("NUMFREEZEBOUNCES", 3)))
        {
            float dot = DotProduct(projectile->velocity, trace.plane.normal);
            VectorMA(projectile->velocity, -2 * dot, trace.plane.normal, projectile->velocity);
            VectorScale(projectile->velocity, freeze ? .8f : .5f, projectile->velocity);
            VectorMA(projectile->s.origin, .125f, trace.plane.normal, projectile->s.origin);
            if (freeze)
            {
                projectile->inventory++;
                projectile->damage /= 2;
            }
        }
        else if (projectile->projectile == DN_PROJECTILE_ROCKET ||
                 projectile->projectile == DN_PROJECTILE_MORTAR)
        {
            VectorMA(projectile->s.origin, .125f, trace.plane.normal, projectile->s.origin);
            Explode(projectile, trace.ent);
            return;
        }
        else
        {
            if (trace.ent && trace.ent != dn_edicts)
            {
                DN_Damage(trace.ent, projectile->owner, projectile->damage, projectile->tile);
            }
            if (projectile->projectile == DN_PROJECTILE_FREEZE ||
                projectile->projectile == DN_PROJECTILE_SHRINK)
            {
                DN_SpawnSprite(
                    projectile,
                    DN_ConSymbol(freeze ? "TRANSPORTERSTAR" : "SHRINKEREXPLOSION", 1647));
            }
            DN_Remove(projectile);
            return;
        }
    }
    if (!charge && dn_time > projectile->firetime)
    {
        DN_Remove(projectile);
        return;
    }
    DN_Link(projectile);
}
