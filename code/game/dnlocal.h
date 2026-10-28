#ifndef DN_LOCAL_H
#define DN_LOCAL_H
#include "q_shared.h"
#define GAME_INCLUDE
#include "game.h"
#include "../qcommon/dnmap.h"
#define DN_MAX_CLIENTS 16
#define DN_MAX_WEAPONS 12
#define DN_CLOCK_HZ 120
#define DN_TICKS_PER_STEP (DN_CLOCK_HZ / 26)
#define DN_SIMULATION_HZ (DN_CLOCK_HZ / DN_TICKS_PER_STEP)
#define DN_TOILET_COOLDOWN (26 * 220)
#define DN_TOILET_ACTIVE_END (26 * 218)

enum
{
    DN_PROJECTILE_ROCKET = 1,
    DN_PROJECTILE_FREEZE,
    DN_PROJECTILE_SHRINK,
    DN_PROJECTILE_PIPEBOMB,
    DN_PROJECTILE_TRIPBOMB,
    DN_PROJECTILE_BEAM,
    DN_PROJECTILE_FIRELASER,
    DN_PROJECTILE_LASERTRAIL,
    DN_PROJECTILE_SPIT,
    DN_PROJECTILE_COOL,
    DN_PROJECTILE_MORTAR
};

struct gclient_s
{
    player_state_t ps;
    int ping;
    usercmd_t lastcmd;
    int health, armor, ammo[DN_MAX_WEAPONS], weapons, weapon, inventory[16];
    int buttons, oldbuttons, noclip, god, connected, kills, secrets, frags;
    float firetime;
    qboolean restore_player;
    int access_wall, access_sprite, access_ticks;
    int weapon_frame, weapon_side, weapon_hold, weapon_position, weapon_sway;
    int transport_hold, jetpack_on;
    int last_pissed_time;
    char name[32];
};

struct edict_s
{
    entity_state_t s;
    gclient_t *client;
    qboolean inuse;
    int linkcount;
    link_t area;
    int num_clusters, clusternums[MAX_ENT_CLUSTERS], headnode, areanum, areanum2;
    int svflags;
    vec3_t mins, maxs, absmin, absmax, size;
    solid_t solid;
    int clipmask;
    edict_t *owner;
    /* Duke actor state; no Quake monster or model state. */
    int tile, originaltile, sector, cstat, xrepeat, yrepeat, pal, shade, lotag, hitag, health;
    int actor, action, move, ai, moveflags, count, actioncount, animframe, animclock;
    int hitweapon, lastweapon, spawnedby, enemy, stayput, inventory, clipdist, sleep;
    int projectile, damage, actiontile, viewtype;
    int demolition_ticks, demolition_direct, lastpal, fragment;
    int transport_hold;
    int dormant, alert_ticks, movflag;
    int faller;
    int zrange_clock;
    int native_state[6], native_angle;
    float cached_floor, cached_ceiling;
    vec3_t lastseen;
    float speed, zspeed, firetime;
    int sound_id;
    float sound_end;
    vec3_t velocity, spawnorigin;
};

extern game_import_t gi;
extern game_export_t globals;
extern edict_t *dn_edicts;
extern dn_map_t dn_world;
extern float dn_time;
extern int dn_maxclients, dn_sprite_model;
void DN_Link(edict_t *ent);
void DN_Remove(edict_t *ent);
edict_t *DN_SpawnSprite(edict_t *parent, int tile);
edict_t *DN_NearestPlayer(edict_t *ent, float *distance);
void DN_Damage(edict_t *target, edict_t *attacker, int damage, int weapon);
void DN_DamageAt(edict_t *target, edict_t *attacker, int damage, int weapon, const float *impact);
void DN_ActorHitPush(edict_t *entity, int damage, int angle);
void DN_InitGib(edict_t *entity);
void DN_SpawnGuts(edict_t *parent, int tile, int count);
int DN_GibTick(edict_t *entity);
void DN_HitBlood(edict_t *target, edict_t *attacker, const float *impact, int weapon);
void DN_RadiusDamage(edict_t *source, int radius, const int *damage, edict_t *direct);
void DN_Shoot(edict_t *ent, int tile);
void DN_ProjectileTick(edict_t *projectile);
void DN_DetonatePipebombs(edict_t *owner);
int DN_PlaceTripbomb(edict_t *player);
void DN_Operate(edict_t *ent);
void DN_OperateActor(edict_t *ent);
int DN_AccessBit(int palette);
int DN_UseSwitch(edict_t *player, int wall, edict_t *sprite);
void DN_ActivateTag(int tag, edict_t *player);
void DN_TickInteractions(void);
void DN_InitTransports(void);
void DN_TickTransports(void);
int DN_CheckPlayerTransport(edict_t *player, const float *previous);
void DN_InitInteractions(void);
void DN_SpriteSize(const edict_t *entity, float *width, float *height);
float DN_ActorHeight(const edict_t *entity);
void DN_PublishWallStyle(int wall);
void DN_PublishSectorStyle(int sector);
void DN_InitStyles(void);
void DN_FreeStyles(void);
int DN_RestoreStyle(edict_t *entity);
int DN_InitDestructible(edict_t *entity);
int DN_DestructibleTick(edict_t *entity);
void DN_TickDestructibles(void);
int DN_DamageDestructible(edict_t *entity, int weapon);
int DN_DamageWall(edict_t *attacker, const float *start, const float *end, const trace_t *trace);
int DN_GlassTick(edict_t *entity);
void DN_TriggerDemolition(int tag);
void DN_TriggerCharges(int sector);
void DN_InitVehicle(edict_t *entity);
int DN_VehicleTick(edict_t *entity);
void DN_InitEffect(edict_t *entity, edict_t *parent);
void DN_InitDecal(edict_t *entity, const edict_t *parent);
int DN_DecalTick(edict_t *entity);
int DN_IsBloodSplat(int tile);
void DN_ShootBlood(edict_t *entity, int tile);
void DN_InitPickup(edict_t *entity, const edict_t *parent);
void DN_SpawnScrap(edict_t *parent, int count);
int DN_FragmentTick(edict_t *entity);
int DN_AllowMapSprite(const dn_sprite_t *sprite);
int DN_SectorMoving(int sector);
void DN_SetWallPosition(int wall, int32_t x, int32_t y);
void DN_PlaySound(edict_t *ent, int id, int once, int global);
void DN_StopSound(edict_t *ent, int id);
int DN_SoundPlaying(edict_t *ent);
void *DN_ReadAsset(const char *relative, size_t *length);
void DN_ChangeLevel(int next);
int DN_IntermissionActive(void);
void DN_InitLevelResults(void);
void DN_CountLevelEnemy(edict_t *entity);
void DN_CheckLevelSector(edict_t *player);
void DN_BeginIntermission(int destination);
void DN_IntermissionFrame(void);
void DN_ContinueIntermission(void);
void DN_SetIntermissionStats(edict_t *player);
void DN_WriteLevelResults(FILE *file);
void DN_ReadLevelResults(FILE *file);
void DN_ResetPlayer(edict_t *ent);
void DN_RespawnPlayer(edict_t *ent);
void DN_WeaponTick(edict_t *ent);
void DN_UpdatePlayerStats(edict_t *ent);
void DN_SelectWeapon(edict_t *ent, int weapon);

/* Shared module state. Keep subsystem-owned data private to its source file. */
extern gclient_t *dn_clients;
extern char dn_current_map[MAX_QPATH];

/* Lifecycle and module boundaries. */
void DN_ResetClock(float time);
void DN_SeedRandom(uint32_t seed);
uint32_t DN_RandomState(void);
int DN_Random(void);
void DN_InitTiles(void);
void DN_InitLighting(void);
int DN_SpriteLighting(const edict_t *entity, int *shade);
int DN_IsEnemy(int tile);
int DN_InitFaller(edict_t *entity);
int DN_DamageFaller(edict_t *entity, int damage, int weapon);
void DN_TickFallers(void);
int DN_IsWallSwitch(int tile);
int DN_IsBathroomFixture(int tile);
void DN_InitMapSwitch(edict_t *entity);
void DN_ActivateLights(int tag, int initialize);
void DN_InitLights(void);
void DN_FreeLights(void);
void DN_TickLights(void);
void DN_WriteLightState(FILE *file);
void DN_ReadLightState(FILE *file);
void DN_InitDropFloors(void);
void DN_FreeDropFloors(void);
void DN_TickDropFloors(void);
void DN_TickTouchplates(void);
int DN_DropFloorMoving(int sector);
int DN_ActivateDropFloor(int sector);
void DN_WriteDropState(FILE *file);
void DN_ReadDropState(FILE *file);
edict_t *DN_NewEntity(void);
void DN_InitSectors(void);
void DN_FreeSectors(void);
void DN_MoveSectors(void);
void DN_PublishSector(int sector);
void DN_ActivateSector(int sector);
void DN_ResetSounds(void);
void DN_SpawnEntities(char *mapname, char *entities, char *spawnpoint);
int DN_IsDeathmatch(void);
int DN_PlayerPalette(const edict_t *player);
void DN_InitMultiplayer(void);
void DN_PublishPlayerScore(const edict_t *player);
void DN_PlayerFrag(edict_t *victim, edict_t *attacker);
edict_t *DN_MapSprite(int index);
void DN_ActorTick(edict_t *ent);
void DN_InitEnemy(edict_t *ent, qboolean map_spawn);
int DN_EnemyDormant(edict_t *ent);
void DN_MoveActor(edict_t *ent, float previous_z);
int DN_DodgeProjectile(edict_t *entity);
int DN_NativeEnemyTick(edict_t *entity);
void DN_FireEnemyProjectile(edict_t *entity, int tile);
void DN_Fall(edict_t *ent);
void DN_ActorZRange(edict_t *entity, float *floor, float *ceiling);
void DN_FireLaser(edict_t *ent);
int DN_CanShoot(edict_t *ent, float distance);
void DN_LaserTick(edict_t *ent);
void DN_ServerCommand(void);
qboolean DN_ClientConnect(edict_t *ent, char *userinfo);
void DN_ClientBegin(edict_t *ent);
void DN_ClientUserinfoChanged(edict_t *ent, char *userinfo);
void DN_ClientDisconnect(edict_t *ent);
void DN_ClientThink(edict_t *ent, usercmd_t *cmd);
void DN_ClientCommand(edict_t *ent);
void DN_WriteSaveData(FILE *file, const void *data, size_t size, size_t count);
void DN_ReadSaveData(FILE *file, void *data, size_t size, size_t count);
void DN_WriteSectorState(FILE *file);
void DN_ReadSectorState(FILE *file);
void DN_RestoreSectorMarkers(void);
void DN_WriteGame(char *filename, qboolean autosave);
void DN_ReadGame(char *filename);
void DN_WriteLevel(char *filename);
void DN_ReadLevel(char *filename);
#endif
