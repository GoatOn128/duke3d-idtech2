/* Build's integer angles and original TABLES.DAT lookup values. */
#ifndef DN_MATH_H
#define DN_MATH_H
#include <stddef.h>

int DN_MathInit(const void *data, size_t size);
int DN_Sin(int angle);
int DN_GetAngle(int x, int y);
int DN_AngleDelta(int angle, int goal);
int DN_SpriteRotation(int angle, int view_angle, int dx, int dy, int views, int *cstat);
#endif
