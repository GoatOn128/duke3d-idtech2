/* CON operations applied to server-owned Duke entities. Keyword numbers and
   meanings follow 3D Realms' GAMEDEF.cpp; movement uses the shared CM/Pmove API. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

static int Flags(const int *args, int argc)
{
    int flags = 0;
    for (int i = 0; i < argc; i++)
    {
        flags |= args[i];
    }
    return flags;
}

static int CanSee(edict_t *e, edict_t *p, int target_only)
{
    vec3_t a, b;
    if (!p)
    {
        return 0;
    }
    VectorCopy(e->s.origin, a);
    a[2] += target_only ? DN_Random() & 41 : (DN_Random() & (47 << 8)) / 256.f;
    VectorCopy(p->s.origin, b);
    b[2] += p->client->ps.viewoffset[2] - (target_only ? 0 : 14);
    int visible = DN_CanSee(&dn_world, a, e->sector, b, p->sector);
    if (!visible && !target_only && !(e->count & 63))
    {
        /* Native furthestcanseepoint samples points visible from the player
           and actor, allowing seekplayer to pursue around a doorway. */
        int increment = dn_maxclients < 2 && gi.cvar("skill", "1", 0)->value < 2
                            ? 1024
                            : 2048 / (1 + (DN_Random() & 1));
        int angle = (int)lroundf(-p->client->ps.viewangles[YAW] * 2048 / 360) & 2047;
        vec3_t start, end, source;
        VectorCopy(p->s.origin, start);
        start[2] += 16 - 28;
        VectorCopy(e->s.origin, source);
        source[2] += 16;
        for (int offset = 0; offset < 2048; offset += increment - (DN_Random() & 511))
        {
            VectorCopy(start, end);
            end[0] += DN_Sin(angle + offset + 512) * 4.f;
            end[1] -= DN_Sin(angle + offset) * 4.f;
            end[2] -= (16384 - (DN_Random() & 32767)) * 4.f / 16;
            trace_t trace = gi.trace(start, vec3_origin, vec3_origin, end, p, MASK_SHOT);
            float from_player =
                fabsf(trace.endpos[0] - start[0]) + fabsf(trace.endpos[1] - start[1]);
            float from_actor =
                fabsf(trace.endpos[0] - source[0]) + fabsf(trace.endpos[1] - source[1]);
            /* Back away from the hit surface so the portal query starts inside. */
            vec3_t point;
            VectorMA(trace.endpos, .0625f, trace.plane.normal, point);
            int sector = DN_FindSectorXY(&dn_world, point, p->sector);
            if (sector >= 0 && from_player < from_actor &&
                DN_CanSee(&dn_world, point, sector, source, e->sector))
            {
                VectorCopy(point, e->lastseen);
                visible = 1;
                break;
            }
        }
    }
    if (visible)
    {
        if (!target_only)
        {
            if (DN_CanSee(&dn_world, a, e->sector, b, p->sector))
            {
                VectorCopy(p->s.origin, e->lastseen);
            }
        }
        e->sleep = 1536;
    }
    return visible;
}

static int PlayerFlags(edict_t *e, edict_t *p)
{
    int flags = 0;
    gclient_t *c;
    if (!p || !p->client)
    {
        return 0;
    }
    c = p->client;
    if (p->xrepeat < 32)
    {
        flags |= 1024;
    }
    if (c->inventory[0] > 0 && c->inventory[0] < 400)
    {
        flags |= 4096;
    }
    if (c->health > 0 && p->xrepeat > 32)
    {
        flags |= 16384;
    }
    else if (c->health <= 0)
    {
        flags |= 32768;
    }
    if (c->ps.pmove.pm_flags & PMF_ON_GROUND)
    {
        flags |= 8192;
        if (abs(c->lastcmd.forwardmove) < 10 && abs(c->lastcmd.sidemove) < 10)
        {
            flags |= 1;
        }
    }
    if (c->ps.pmove.pm_flags & PMF_DUCKED)
    {
        flags |= 8;
    }
    if (c->lastcmd.forwardmove > 0)
    {
        flags |= abs(c->lastcmd.forwardmove) > 200 ? 4 : 2;
    }
    if (c->lastcmd.forwardmove < 0)
    {
        flags |= abs(c->lastcmd.forwardmove) > 200 ? 256 : 128;
    }
    if (c->ps.pmove.velocity[2] < -16)
    {
        flags |= 16;
    }
    if (c->ps.pmove.velocity[2] > 16)
    {
        flags |= 32;
    }
    if (p->s.origin[2] + c->ps.viewoffset[2] > e->s.origin[2] + 48)
    {
        flags |= 64;
    }
    if (c->jetpack_on)
    {
        flags |= 2048;
    }
    if (c->weapon == 0 && c->buttons & BUTTON_ATTACK)
    {
        flags |= 512;
    }
    float desired = atan2f(e->s.origin[1] - p->s.origin[1], e->s.origin[0] - p->s.origin[0]) * 180 /
                    (float)M_PI;
    if (fabsf(anglemod(desired - c->ps.viewangles[1] + 180) - 180) < 22.5f)
    {
        flags |= 65536;
    }
    return flags;
}

int DN_ConCondition(edict_t *e, int op, const int *a, int argc)
{
    float distance;
    edict_t *p = DN_NearestPlayer(e, &distance);
    gclient_t *c = p ? p->client : NULL;
    int sector = e->sector;
    dn_sector_t *s = sector >= 0 ? &dn_world.sectors[sector] : NULL;
    switch (op)
    {
    case DN_OP_IFRND:
        return (int)(DN_Random() >> 8) >= 255 - a[0];
    case DN_OP_IFCANSEE:
        return CanSee(e, p, 0);
    case DN_OP_IFCANSEETARGET:
        return CanSee(e, p, 1);
    case DN_OP_IFCANSHOOTTARGET:
        return DN_CanShoot(e, distance);
    case DN_OP_IFHITWEAPON:
    {
        int hit = e->hitweapon;
        e->hitweapon = 0;
        return hit;
    }
    case DN_OP_IFPDISTL:
        if (distance > 16384.f / 16 && !e->sleep)
        {
            e->sleep = 1536;
        }
        return distance < a[0] / 16.f;
    case DN_OP_IFPDISTG:
        if (distance > 16384.f / 16 && !e->sleep)
        {
            e->sleep = 1536;
        }
        return distance > a[0] / 16.f;
    case DN_OP_IFAI:
        return e->ai == a[0];
    case DN_OP_IFDEAD:
        return e->client ? e->health <= 0 : e->health < 0;
    case DN_OP_IFSQUISHED:
        return s && DN_PlaneZ(&dn_world, sector, 1, e->s.origin[0], e->s.origin[1]) -
                            DN_PlaneZ(&dn_world, sector, 0, e->s.origin[0], e->s.origin[1]) <
                        e->maxs[2] - e->mins[2] - 1;
    case DN_OP_IFWASWEAPON:
        return e->lastweapon == a[0];
    case DN_OP_IFACTION:
        return e->action == a[0];
    case DN_OP_IFACTIONCOUNT:
        return e->actioncount >= a[0];
    case DN_OP_IFMOVE:
        return e->move == a[0];
    case DN_OP_IFONWATER:
        return s && s->lotag == 1 && fabsf(e->s.origin[2] - s->floorz / -256.f) < 32;
    case DN_OP_IFINWATER:
        return s && s->lotag == 2;
    case DN_OP_IFCOUNT:
        return e->count >= a[0];
    case DN_OP_IFACTORNOTSTAYPUT:
        return e->stayput < 0;
    case DN_OP_IFP:
        return !!(PlayerFlags(e, p) & Flags(a, argc));
    case DN_OP_IFACTOR:
        return e->tile == a[0];
    case DN_OP_IFSTRENGTH:
        return e->health <= a[0];
    case DN_OP_IFSPAWNEDBY:
        return e->spawnedby == a[0];
    case DN_OP_IFGAPZL:
        return s && DN_PlaneZ(&dn_world, sector, 1, e->s.origin[0], e->s.origin[1]) -
                            DN_PlaneZ(&dn_world, sector, 0, e->s.origin[0], e->s.origin[1]) <
                        a[0];
    case DN_OP_IFHITSPACE:
        return c && (c->buttons & BUTTON_USE);
    case DN_OP_IFOUTSIDE:
        return s && (s->ceilingstat & 1);
    case DN_OP_IFMULTIPLAYER:
        return dn_maxclients > 1;
    case DN_OP_IFINSPACE:
    case DN_OP_IFINOUTERSPACE:
    {
        if (!s || s->ceilingpal != 0)
        {
            return 0;
        }
        int floor = op == DN_OP_IFINOUTERSPACE;
        int tile = floor ? s->floorpicnum : s->ceilingpicnum;
        return ((floor ? s->floorstat : s->ceilingstat) & 1) &&
               (tile == DN_ConSymbol("MOONSKY1", 80) || tile == DN_ConSymbol("BIGORBIT1", 84));
    }
    case DN_OP_IFBULLETNEAR:
        return DN_DodgeProjectile(e);
    case DN_OP_IFRESPAWN:
        return gi.cvar((char *)(e->enemy ? "respawn_monsters" : "respawn_items"), "0", 0)->value != 0;
    case DN_OP_IFFLOORDISTL:
    case DN_OP_IFCEILINGDISTL:
    {
        float floor, ceiling;
        DN_ActorZRange(e, &floor, &ceiling);
        return s && (op == DN_OP_IFFLOORDISTL ? e->s.origin[2] - floor
                                              : ceiling - e->s.origin[2]) <= a[0];
    }
    case DN_OP_IFPINVENTORY:
        if (c && a[0] == 6)
        {
            return (c->inventory[6] & DN_AccessBit(e->pal)) != 0;
        }
        if (c && a[0] == 1)
        {
            return c->armor != DN_ConSetting("MAXPLAYERHEALTH", 100);
        }
        return c && a[0] >= 0 && a[0] < 16 && (a[0] == 1 ? c->armor : c->inventory[a[0]]) != a[1];
    case DN_OP_IFPHEALTHL:
        return c && c->health < a[0];
    case DN_OP_IFNOTMOVING:
        return (e->movflag & 49152) > 16384;
    case DN_OP_IFSPRITEPAL:
        return e->pal == a[0];
    case DN_OP_IFAWAYFROMWALL:
    {
        vec3_t mins = {-8, -8, 0}, maxs = {8, 8, 1};
        trace_t tr = gi.trace(e->s.origin, mins, maxs, e->s.origin, e, MASK_SOLID);
        return !tr.startsolid;
    }
    case DN_OP_IFGOTWEAPONCE:
        return gi.cvar("coop", "0", 0)->value != 0 && c && (c->weapons & (1 << c->weapon));
    case DN_OP_IFNOSOUNDS:
        return !DN_SoundPlaying(e);
    case DN_OP_IFANGDIFFL:
        if (p)
        {
            return fabsf(anglemod(c->ps.viewangles[1] - e->s.angles[1] + 180) - 180) <=
                   a[0] * 360.f / 2048;
        }
        return 0;
    default:
        gi.dprintf("Unhandled CON condition %d\n", op);
        return 0;
    }
}

static void SetAction(edict_t *e, int action)
{
    e->action = action;
    e->actioncount = e->animframe = e->animclock = 0;
}

static void SetMove(edict_t *e, int move, int flags)
{
    e->move = move;
    e->moveflags = flags;
    e->count = 0;
    if (flags & 8)
    {
        e->s.angles[1] = -(DN_Random() & 2047) * 360.f / 2048;
    }
}

static void Fragments(edict_t *e, int tile, int count)
{
    if (count > 64)
    {
        count = 64;
    }
    for (int i = 0; i < count; i++)
    {
        edict_t *g = DN_SpawnSprite(e, tile);
        g->xrepeat = g->yrepeat = 8;
        g->cstat = 0;
        g->s.origin[0] += (DN_Random() % 24) - 12;
        g->s.origin[1] += (DN_Random() % 24) - 12;
        g->s.origin[2] += DN_Random() % 24;
        DN_Link(g);
    }
}

int DN_ConCommand(edict_t *e, int op, const int *a, int argc)
{
    edict_t *p = DN_NearestPlayer(e, NULL);
    gclient_t *c = p ? p->client : NULL;
    switch (op)
    {
    case DN_OP_ACTION:
        SetAction(e, a[0]);
        break;
    case DN_OP_AI:
        if (a[0] >= 0 && a[0] < 1024)
        {
            dn_ai_t *ai = &dn_ais[a[0]];
            e->ai = a[0];
            SetAction(e, ai->action);
            SetMove(e, ai->move, ai->flags);
        }
        break;
    case DN_OP_MOVE:
        SetMove(e, a[0], argc > 1 ? Flags(a + 1, argc - 1) : 0);
        break;
    case DN_OP_STRENGTH:
        e->health = a[0];
        break;
    case DN_OP_ADDSTRENGTH:
        e->health += a[0];
        break;
    case DN_OP_BREAK:
        return 0;
    case DN_OP_KILLIT:
        DN_Remove(e);
        return 0;
    case DN_OP_SHOOT:
        DN_Shoot(e, a[0]);
        break;
    case DN_OP_PALFROM:
        if (c)
        {
            c->ps.blend[0] = argc > 1 ? a[1] / 63.f : 1;
            c->ps.blend[1] = argc > 2 ? a[2] / 63.f : 0;
            c->ps.blend[2] = argc > 3 ? a[3] / 63.f : 0;
            c->ps.blend[3] = fminf(1, a[0] / 64.f);
        }
        break;
    case DN_OP_SOUND:
    case DN_OP_SOUNDONCE:
    case DN_OP_GLOBALSOUND:
        DN_PlaySound(e, a[0], op == DN_OP_SOUNDONCE, op == DN_OP_GLOBALSOUND);
        break;
    case DN_OP_STOPSOUND:
        DN_StopSound(e, a[0]);
        break;
    case DN_OP_FALL:
        DN_Fall(e);
        break;
    case DN_OP_SPAWN:
        DN_SpawnSprite(e, a[0]);
        break;
    case DN_OP_CACTOR:
        if (a[0] >= 0 && a[0] < DN_MAX_TILES)
        {
            e->tile = a[0];
            const dn_actor_t *actor = DN_ConActor(e->tile);
            e->actor = actor ? actor->code : -1;
        }
        break;
    case DN_OP_SPRITEPAL:
        if (!e->client)
        {
            e->lastpal = e->pal;
        }
        e->pal = a[0] & 255;
        break;
    case DN_OP_GETLASTPAL:
        e->pal = e->client ? DN_PlayerPalette(e) : e->lastpal;
        e->lastpal = 0;
        break;
    case DN_OP_CSTAT:
        e->cstat = a[0];
        break;
    case DN_OP_CSTATOR:
        e->cstat |= a[0];
        break;
    case DN_OP_CLIPDIST:
        e->clipdist = a[0];
        break;
    case DN_OP_SIZEAT:
        e->xrepeat = a[0];
        e->yrepeat = a[1];
        break;
    case DN_OP_SIZETO:
        if (e->xrepeat < a[0])
        {
            e->xrepeat++;
        }
        else if (e->xrepeat > a[0])
        {
            e->xrepeat--;
        }
        if (e->yrepeat < a[1])
        {
            e->yrepeat++;
        }
        else if (e->yrepeat > a[1])
        {
            e->yrepeat--;
        }
        break;
    case DN_OP_RESETACTIONCOUNT:
        e->actioncount = 0;
        break;
    case DN_OP_RESETCOUNT:
        e->count = 0;
        break;
    case DN_OP_COUNT:
        e->count = a[0];
        break;
    case DN_OP_ADDAMMO:
    case DN_OP_ADDWEAPON:
        if (c && a[0] >= 0 && a[0] < DN_MAX_WEAPONS)
        {
            int max = 200;
            static const char *limits[] = {"",
                                           "MAXPISTOLAMMO",
                                           "MAXSHOTGUNAMMO",
                                           "MAXCHAINGUNAMMO",
                                           "MAXRPGAMMO",
                                           "MAXHANDBOMBAMMO",
                                           "MAXSHRINKERAMMO",
                                           "MAXDEVISTATORAMMO",
                                           "MAXTRIPBOMBAMMO",
                                           "MAXFREEZEAMMO",
                                           "",
                                           "MAXGROWAMMO"};
            max = DN_ConSetting(limits[a[0]], 200);
            int owned = (c->weapons & (1 << a[0])) != 0;
            if (c->ammo[a[0]] >= max && (op == DN_OP_ADDAMMO || owned))
            {
                return 0;
            }
            if (op == DN_OP_ADDWEAPON)
            {
                c->weapons |= 1 << a[0];
                if (!owned || c->weapon == DN_WEAPON_KNEE)
                {
                    DN_SelectWeapon(p, a[0]);
                }
            }
            c->ammo[a[0]] += a[1];
            if (c->ammo[a[0]] > max)
            {
                c->ammo[a[0]] = max;
            }
        }
        break;
    case DN_OP_ADDPHEALTH:
        if (c)
        {
            int max = DN_ConSetting("MAXPLAYERHEALTH", 100);
            if (e->tile == DN_ConSymbol("ATOMICHEALTH", 100))
            {
                max *= 2;
            }
            if (a[0] > 0 && c->health >= max)
            {
                break;
            }
            if (c->health > 0 && !c->god)
            {
                c->health += a[0];
            }
            c->health = fmaxf(0, c->health);
            if (a[0] > 0 && c->health > max)
            {
                c->health = max;
            }
            p->health = c->health;
        }
        break;
    case DN_OP_ADDINVENTORY:
        if (c && a[0] >= 0 && a[0] < 16)
        {
            if (a[0] == 1)
            {
                c->armor = a[1];
            }
            else if (a[0] == 6)
            {
                c->inventory[6] |= DN_AccessBit(e->pal);
            }
            else
            {
                c->inventory[a[0]] = a[1];
            }
        }
        break;
    case DN_OP_QUOTE:
        if (p && a[0] >= 0 && a[0] < 256)
        {
            gi.centerprintf(p, "%s", dn_quotes[a[0]]);
        }
        break;
    case DN_OP_RESETPLAYER:
        if (p)
        {
            DN_RespawnPlayer(p);
        }
        break;
    case DN_OP_ADDKILLS:
        e->stayput = -1;
        if (c)
        {
            c->kills += a[0];
        }
        break;
    case DN_OP_HITRADIUS:
        DN_RadiusDamage(e, a[0], a + 1, NULL);
        break;
    case DN_OP_PSTOMP:
    case DN_OP_PKICK:
        if (p)
        {
            DN_Damage(e, p, 20, DN_ConSymbol("KNEE", 2521));
        }
        break;
    case DN_OP_WACKPLAYER:
        if (c)
        {
            c->ps.kick_angles[0] = -8;
        }
        break;
    case DN_OP_OPERATE:
        DN_OperateActor(e);
        break;
    case DN_OP_ENDOFGAME:
        DN_ChangeLevel(1);
        break;
    case DN_OP_DEBUG:
        gi.dprintf("CON debug %d, actor %d\n", a[0], e->tile);
        break;
    case DN_OP_DEBRIS:
        Fragments(e, a[0], a[1]);
        break;
    case DN_OP_GUTS:
        DN_SpawnGuts(e, a[0], a[1]);
        break;
    case DN_OP_LOTSOFGLASS:
        Fragments(e, DN_ConSymbol("GLASSPIECES", 1031), a[0]);
        break;
    case DN_OP_MONEY:
    case DN_OP_MAIL:
    case DN_OP_PAPER:
        Fragments(e,
                  DN_ConSymbol(op == DN_OP_MONEY  ? "MONEY"
                               : op == DN_OP_MAIL ? "MAIL"
                                                  : "PAPER",
                               1233),
                  a[0]);
        break;
    case DN_OP_RESPAWNHITAG:
        if (e->hitag > 0 && e->hitag < DN_MAX_TILES)
        {
            DN_SpawnSprite(e, e->hitag);
        }
        break;
    case DN_OP_TOSSWEAPON:
        if (c)
        {
            DN_SpawnSprite(p, DN_ConSymbol("FIRSTGUNSPRITE", 21));
        }
        break;
    case DN_OP_SLEEPTIME:
        e->sleep = a[0];
        break;
    case DN_OP_TIP:
        if (c)
        {
            c->ps.kick_angles[0] = 4;
        }
        break;
    case DN_OP_MIKESND:
        DN_PlaySound(e, e->lotag, 1, 0);
        break;
    case DN_OP_NULLOP:
        break;
    default:
        gi.dprintf("Unhandled CON operation %d on tile %d\n", op, e->tile);
        break;
    }
    return e->inuse;
}
