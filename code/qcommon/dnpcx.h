/* Bounded, row-stride-aware decoding for indexed PCX artwork. */
#ifndef DN_PCX_H
#define DN_PCX_H
#include <stddef.h>
int DN_DecodePCX(const void *data, size_t size, unsigned char **pixels, int *width, int *height,
                 unsigned char *palette);
#endif
