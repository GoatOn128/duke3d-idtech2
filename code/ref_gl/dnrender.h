#ifndef DN_RENDER_H
#define DN_RENDER_H
#include "../qcommon/dnmap.h"
extern dn_map_t r_duke_map;
void R_DukeLoad(model_t *model, void *buffer, int length);
void R_DukeFree(void);
void R_DukeRegister(void);
void R_DukeDrawWorld(void);
void R_DukeDrawSprite(entity_t *entity);
void R_DukeDrawWeapon(entity_t *entity);
void R_DukeWeaponInfo_f(void);
void R_DukeApplyWorldStates(const refdef_t *frame);
void R_DukeWorldInfo_f(void);
void R_DukeTextureCheck_f(void);
void R_DukeLight(const float *point, float *color);
#endif
