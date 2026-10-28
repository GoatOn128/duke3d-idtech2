/* Cosmetic weapon clock prediction. Damage, ammo and sounds remain server-owned. */
#include "cgame.h"

void DN_PredictWeapon(const dn_weapon_view_t *source, int ammo, int attack, int ticks,
                      dn_weapon_view_t *view)
{
    *view = *source;
    if (view->weapon < 0 || view->weapon >= DN_WEAPON_COUNT || view->phase < 0 ||
        view->phase > 31 || ticks <= 0)
    {
        return;
    }
    if (ticks > DN_ANIMATION_MAX_PREDICTION)
    {
        ticks = DN_ANIMATION_MAX_PREDICTION;
    }
    for (int tick = 0; tick < ticks; tick++)
    {
        if (view->position < 0)
        {
            view->position = DN_Max(-9, view->position - 1);
            continue;
        }
        if (view->position > 0)
        {
            view->position--;
            continue;
        }
        if (view->access)
        {
            return;
        }
        int *phase = &view->phase;
        if (!*phase)
        {
            if (!attack ||
                (view->weapon != DN_WEAPON_KNEE && view->weapon != DN_WEAPON_REMOTE && ammo <= 0))
            {
                return;
            }
            *phase = 1;
            if (view->weapon == DN_WEAPON_PISTOL)
            {
                ammo--;
            }
            if (view->weapon == DN_WEAPON_DEVASTATOR)
            {
                view->side ^= 1;
            }
            continue;
        }
        switch (view->weapon)
        {
        case DN_WEAPON_PISTOL:
            ++*phase;
            if ((*phase >= 5 && (ammo <= 0 || ammo % 12)) || *phase == 27)
            {
                *phase = 0;
            }
            break;
        case DN_WEAPON_SHOTGUN:
            ++*phase;
            if (*phase == 4)
            {
                ammo--;
            }
            if (*phase == 17 || *phase == 20 || *phase == 24)
            {
                ++*phase;
            }
            if (*phase == 31)
            {
                *phase = 0;
            }
            break;
        case DN_WEAPON_CHAINGUN:
            ++*phase;
            if (*phase <= 12 && *phase % 3 == 0)
            {
                if (ammo-- <= 0 || !attack)
                {
                    *phase = 0;
                }
            }
            else if (*phase > 12)
            {
                *phase = attack ? 1 : 0;
            }
            break;
        case DN_WEAPON_RPG:
            if (++*phase == 4)
            {
                ammo--;
            }
            if (*phase == 20)
            {
                *phase = 0;
            }
            break;
        case DN_WEAPON_SHRINKER:
        case DN_WEAPON_GROW:
            if (*phase > (view->weapon == DN_WEAPON_GROW ? 3 : 10))
            {
                ammo--;
                *phase = 0;
            }
            else
            {
                ++*phase;
            }
            break;
        case DN_WEAPON_DEVASTATOR:
            if (++*phase & 1)
            {
                ammo--;
            }
            if (*phase > 5)
            {
                *phase = 0;
            }
            break;
        case DN_WEAPON_FREEZE:
            if (*phase < 4)
            {
                if (++*phase == 3)
                {
                    ammo--;
                }
            }
            else
            {
                *phase = attack && ammo > 0 ? 1 : 0;
            }
            break;
        case DN_WEAPON_KNEE:
            /* The repeating kick chooses a frame from the shared server RNG. */
            if (*phase == 13 && attack)
            {
                return;
            }
            if (++*phase == 14)
            {
                *phase = 0;
            }
            break;
        case DN_WEAPON_PIPEBOMB:
            if ((*phase == 6 && attack) || *phase >= 19)
            {
                return;
            }
            ++*phase;
            break;
        case DN_WEAPON_REMOTE:
            if (*phase >= 9)
            {
                return; /* The server selects the next owned weapon. */
            }
            ++*phase;
            break;
        case DN_WEAPON_TRIPBOMB:
            if (*phase == 3)
            {
                return; /* Placement succeeds or fails in the authoritative world. */
            }
            if (++*phase > 16)
            {
                *phase = 0;
            }
            break;
        }
    }
}
