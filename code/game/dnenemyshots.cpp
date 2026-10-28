/* Travelling Octabrain/Battlelord blasts, Enforcer spit and lobbed mortars. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

void DN_FireEnemyProjectile(edict_t *entity, int tile)
{
    edict_t *player = DN_NearestPlayer(entity, NULL);
    if (!player)
    {
        return;
    }
    int spit = tile == DN_ConSymbol("SPIT", 1636);
    int cool = tile == DN_ConSymbol("COOLEXPLOSION1", 1360);
    int speed = spit ? 292 : cool ? (entity->tile == DN_ConSymbol("BOSS2", 2710) ? 644 : 348) : 0;
    int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
    vec3_t start;
    VectorCopy(entity->s.origin, start);
    start[2] += DN_ActorHeight(entity) / 2 + 3;
    float dx = player->s.origin[0] - start[0], dy = player->s.origin[1] - start[1];
    int distance = DN_Max(1, (int)(sqrtf(dx * dx + dy * dy) * 16));
    int vertical;
    if (spit || cool)
    {
        start[2] += cool ? 2 : 0;
        angle += 16 - (DN_Random() & 31);
        vertical = (int)((start[2] - player->s.origin[2] - player->client->ps.viewoffset[2] + 3) *
                         256 * speed / distance);
        if (spit)
        {
            start[2] += 10;
        }
    }
    else
    {
        speed = distance >> 4;
        vertical = -(distance >> 1);
        if (vertical < -4096)
        {
            vertical = -2048;
        }
        start[0] += (DN_Sin(angle + 1536) >> 8) / 16.f;
        start[1] -= (DN_Sin(angle + 512) >> 8) / 16.f;
        start[2] -= 6;
    }
    edict_t *shot = DN_SpawnSprite(entity, tile);
    shot->actor = -1;
    shot->projectile = spit ? DN_PROJECTILE_SPIT : cool ? DN_PROJECTILE_COOL : DN_PROJECTILE_MORTAR;
    shot->damage = shot->health + ((spit || cool) ? (DN_Random() & 7) : 0);
    shot->cstat = spit || cool ? 128 : 0;
    shot->clipdist = 4;
    shot->xrepeat = shot->yrepeat = spit || cool ? 18 : 32;
    shot->shade = cool ? 0 : -64;
    shot->firetime = dn_time + 8;
    shot->s.angles[YAW] = -(angle & 2047) * 360.f / 2048;
    VectorCopy(start, shot->s.origin);
    shot->velocity[0] = ((speed * DN_Sin(angle + 512)) >> 14) * DN_SIMULATION_HZ / 32.f;
    shot->velocity[1] = -((speed * DN_Sin(angle)) >> 14) * DN_SIMULATION_HZ / 32.f;
    shot->velocity[2] = -vertical * DN_SIMULATION_HZ / 512.f;
    DN_Link(shot);
}
