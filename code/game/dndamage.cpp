/* Radius damage eligibility and randomized thirds from original hitradius(). */
#include "dncon.h"

void DN_RadiusDamage(edict_t *source, int radius, const int *damage, edict_t *direct)
{
    if (radius <= 0)
    {
        return;
    }
    for (int i = 1; i < globals.num_edicts; i++)
    {
        edict_t *target = &dn_edicts[i];
        if (!target->inuse || target == source || target == direct ||
            (target->s.renderfx & (RF_DUKE_WORLD | RF_BEAM)) ||
            (!target->client && !target->enemy && !(target->cstat & 257)))
        {
            continue;
        }
        vec3_t delta, origin, endpoint;
        VectorSubtract(target->s.origin, source->s.origin, delta);
        float distance = VectorLength(delta), reach = radius / 16.f;
        if (distance >= reach)
        {
            continue;
        }
        int standable = target->tile == DN_ConSymbol("SEENINE", 1247) ||
                        target->tile == DN_ConSymbol("OOZFILTER", 1079) ||
                        (target->tile >= DN_ConSymbol("CRACK1", 546) &&
                         target->tile <= DN_ConSymbol("CRACK4", 549));
        if (target->client || target->enemy || standable)
        {
            VectorCopy(source->s.origin, origin);
            VectorCopy(target->s.origin, endpoint);
            origin[2] += 12;
            endpoint[2] += 8;
            if (!DN_CanSee(&dn_world, origin, source->sector, endpoint, target->sector))
            {
                continue;
            }
        }
        int band = distance < reach / 3 ? 2 : distance < reach * 2 / 3 ? 1 : 0;
        int low = damage[band], high = damage[band + 1];
        int amount = low + DN_Random() % (high > low ? high - low : 1);
        DN_Damage(target, source->owner ? source->owner : source, amount,
                  DN_ConSymbol("RADIUSEXPLOSION", 1670));
    }
}
