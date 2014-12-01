
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#include "image.h"
#include "geometry.h"

Geometry* parseLayout(const Image* const img, const float scaling);

char* buildCollisionMap(const Image* const img);

#endif

