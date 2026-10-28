#ifndef DN_PREDICTION_H
#define DN_PREDICTION_H
#include <stdint.h>
#define DN_PREDICTION_VERSION 3

typedef struct
{
#define DN_PREDICTION_FIELD(type, name) type name;
#include "dnprediction.inc"
#undef DN_PREDICTION_FIELD
} dn_prediction_t;
#endif
