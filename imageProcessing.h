#error THIS FILE IS NOT CURRENTLY USED!

#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vector3_cl.h"
#include <stdint.h>

void selectiveDilate(uint8_t *data, int width, int height);
void subsampleAndConvertToPerceptive(Vector3* lights, uint8_t *dataOut, int width, int height);


#ifdef __cplusplus
}
#endif


#endif

