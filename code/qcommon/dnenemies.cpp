/* Native badguypic classification shared by game logic and movement prediction. */
#include "dncon.h"

static const char *enemy_names[] = {"SHARK",
                                    "RECON",
                                    "DRONE",
                                    "LIZTROOPONTOILET",
                                    "LIZTROOPJUSTSIT",
                                    "LIZTROOPSTAYPUT",
                                    "LIZTROOPSHOOT",
                                    "LIZTROOPJETPACK",
                                    "LIZTROOPDUCKING",
                                    "LIZTROOPRUNNING",
                                    "LIZTROOP",
                                    "OCTABRAIN",
                                    "OCTABRAINSTAYPUT",
                                    "COMMANDER",
                                    "COMMANDERSTAYPUT",
                                    "PIGCOP",
                                    "EGG",
                                    "PIGCOPSTAYPUT",
                                    "PIGCOPDIVE",
                                    "LIZMAN",
                                    "LIZMANSTAYPUT",
                                    "LIZMANSPITTING",
                                    "LIZMANFEEDING",
                                    "LIZMANJUMP",
                                    "ORGANTIC",
                                    "BOSS1",
                                    "BOSS1STAYPUT",
                                    "BOSS2",
                                    "BOSS3",
                                    "BOSS4",
                                    "BOSS4STAYPUT",
                                    "GREENSLIME",
                                    "RAT",
                                    "ROTATEGUN"};

int DN_IsEnemy(int tile)
{
    const dn_actor_t *actor = DN_ConActor(tile);
    if (actor && actor->enemy)
    {
        return 1;
    }
    int slime = DN_ConSymbol("GREENSLIME", 2370);
    if (tile >= slime && tile <= slime + 7)
    {
        return 1;
    }
    for (int i = 0; i < sizeof(enemy_names) / sizeof(enemy_names[0]); i++)
    {
        if (tile == DN_ConSymbol(enemy_names[i], -1))
        {
            return 1;
        }
    }
    return 0;
}
