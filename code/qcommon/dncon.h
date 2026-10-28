#ifndef DN_CON_H
#define DN_CON_H
int DN_IsEnemy(int tile);
#include <stddef.h>
#include <stdint.h>

enum
{

#define DN_KEYWORD(name, text) DN_OP_##name,
#include "dnkeywords.inc"
#undef DN_KEYWORD
    DN_OP_JUMP,
    DN_OP_RETURN
};

typedef struct
{
    int value[5];
} dn_action_t;

typedef struct
{
    int speed, zspeed;
} dn_move_t;

typedef struct
{
    int action, move, flags;
} dn_ai_t;

typedef struct
{
    int tile, code, health, action, move, flags, enemy;
} dn_actor_t;

typedef struct
{
    char file[64];
    int pitchlow, pitchhigh, priority, flags, distance;
} dn_sound_t;

typedef struct
{
    char name[64];
    int par_seconds, designer_seconds;
} dn_level_info_t;

extern dn_action_t dn_actions[2048];
extern dn_move_t dn_moves[1024];
extern dn_ai_t dn_ais[1024];
extern dn_sound_t dn_sounds[1024];
extern char dn_quotes[256][192];
int DN_ConLoad(void);
void DN_ConFree(void);
int DN_ConSymbol(const char *name, int fallback);
int DN_ConSetting(const char *name, int fallback);
const dn_actor_t *DN_ConActor(int tile);
const dn_level_info_t *DN_ConLevel(int episode, int level);
const char *DN_ConMusic(int episode, int level);

typedef struct
{
    void *(*ReadFile)(const char *path, size_t *length);
    void (*Print)(char *format, ...);
    void (*Error)(char *format, ...);
} dn_con_import_t;

typedef struct
{
    void *state;
    int tile, entry;
    int (*Alive)(void *state);
    int (*Condition)(void *state, int op, const int *args, int argc);
    int (*Command)(void *state, int op, const int *args, int argc);
} dn_con_vm_t;

void DN_ConConfigure(const dn_con_import_t *imports);
uint32_t DN_ConChecksum(void);
int DN_ConExecute(const dn_con_vm_t *vm);
#endif
