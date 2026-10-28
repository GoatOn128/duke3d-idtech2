/* Versioned boundary between the Quake client and Duke client-game module. */
#ifndef DN_CGAME_H
#define DN_CGAME_H
#include "../game/q_shared.h"
#include "../qcommon/dnmap.h"
#include "../qcommon/dncon.h"
#include "dnweaponview.h"
#define DN_CGAME_API_VERSION 5

typedef struct
{
    uint32_t version, size;
    void (*Print)(char *format, ...);
    void (*Error)(char *format, ...);
    void *(*ReadFile)(const char *path, size_t *length);
    void (*Pmove)(pmove_t *move);
    const dn_map_t *(*CollisionMap)(void);
} dn_cgame_import_t;

typedef struct
{
    uint32_t version, size;
    int (*Init)(void);
    void (*Shutdown)(void);
    uint32_t (*ConChecksum)(void);
    void (*PredictMove)(pmove_t *move, dn_prediction_t *state);
    void (*BuildWeaponView)(const dn_weapon_view_t *view, dn_weapon_sprites_t *sprites);
    int (*ExecuteCon)(const dn_con_vm_t *vm);
    int (*RandomNext)(uint32_t *seed);
    int (*PredictAnimation)(const entity_state_t *state, uint32_t target_tick);
    void (*PredictWeapon)(const dn_weapon_view_t *source, int ammo, int attack, int ticks,
                          dn_weapon_view_t *view);
    int (*SoundInfo)(int id, dn_sound_t *sound);
} dn_cgame_export_t;

typedef dn_cgame_export_t *(*dn_get_cgame_api_t)(const dn_cgame_import_t *imports);
int DN_PredictAnimation(const entity_state_t *state, uint32_t target_tick);
void DN_PredictWeapon(const dn_weapon_view_t *source, int ammo, int attack, int ticks,
                      dn_weapon_view_t *view);
#endif
