#ifndef DN_TRANSPORT_H
#define DN_TRANSPORT_H
#include "dnmap.h"

typedef struct
{
    int sector, hold, jetpack, grounded, jumping, upmove;
    float eyeheight;
    float point[3], previous[3];
    int has_previous;
} dn_transport_query_t;

typedef struct
{
    int source, destination, onfloor, hold;
    float point[3], yaw;
} dn_transport_result_t;

void DN_InitTransportLinks(dn_map_t *map);
int DN_QueryPlayerTransport(const dn_map_t *map, const dn_transport_query_t *query,
                            dn_transport_result_t *result);
#endif
