
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif


#include "image.h"
#include "geometry.h"

Geometry* parseLayout(const Image* const img, const float scaling, const float TILE_SIZE);
char* buildCollisionMap(const Image* const img);

#ifdef __cplusplus
}
#endif


#endif

