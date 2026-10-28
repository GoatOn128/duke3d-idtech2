/* Integer math from Ken Silverman's Build ENGINE.cpp, GPL-2.0-or-later. */
#include "dnmath.h"
#include <stdint.h>
#include <stdlib.h>

static int16_t sine[2048], radar[1280];

static int ReadShort(const uint8_t *data)
{
    return (int16_t)(data[0] | (data[1] << 8));
}

int DN_MathInit(const void *data, size_t size)
{
    if (!data || size < 4096 + 1280)
    {
        return 0;
    }
    const uint8_t *bytes = (const uint8_t *)(data);
    for (int i = 0; i < 2048; i++)
    {
        sine[i] = ReadShort(bytes + i * 2);
    }
    for (int i = 0; i < 640; i++)
    {
        radar[i] = ReadShort(bytes + 4096 + i * 2);
        radar[1279 - i] = -radar[i];
    }
    return 1;
}

int DN_Sin(int angle)
{
    return sine[angle & 2047];
}

int DN_GetAngle(int x, int y)
{
    if (!(x | y))
    {
        return 0;
    }
    if (!x)
    {
        return 512 + ((y < 0) << 10);
    }
    if (!y)
    {
        return (x < 0) << 10;
    }
    if (x == y)
    {
        return 256 + ((x < 0) << 10);
    }
    if (x == -y)
    {
        return 768 + ((x > 0) << 10);
    }
    if (abs(x) > abs(y))
    {
        return ((radar[640 + (int)((int64_t)160 * y / x)] >> 6) + ((x < 0) << 10)) & 2047;
    }
    return ((radar[640 - (int)((int64_t)160 * x / y)] >> 6) + 512 + ((y < 0) << 10)) & 2047;
}

int DN_AngleDelta(int angle, int goal)
{
    angle &= 2047;
    goal &= 2047;
    if (abs(angle - goal) >= 1024)
    {
        if (goal > 1024)
        {
            goal -= 2048;
        }
        if (angle > 1024)
        {
            angle -= 2048;
        }
    }
    return goal - angle;
}

int DN_SpriteRotation(int angle, int view_angle, int dx, int dy, int views, int *cstat)
{
    int direction = 0;
    switch (views)
    {
    case 2:
        direction = (((angle + 3072 + 128 - view_angle) & 2047) >> 8) & 1;
        break;
    case 3:
    case 4:
        direction = ((angle + 3072 + 128 - view_angle) & 2047) >> 7;
        direction &= 7;
        *cstat &= ~4;
        if (direction > 3)
        {
            direction = 7 - direction;
            *cstat |= 4;
        }
        break;
    case 5:
    case 7:
        direction = (angle + 3072 + 128 - DN_GetAngle(dx, dy)) & 2047;
        direction = views == 5 ? direction >> 8 : direction / 170;
        *cstat &= ~4;
        if (direction > views - 1)
        {
            direction = 2 * (views - 1) - direction;
            *cstat |= 4;
        }
        break;
    case 8:
        direction = ((angle + 3072 + 128 - view_angle) & 2047) >> 8;
        *cstat &= ~4;
        break;
    }
    return direction;
}
