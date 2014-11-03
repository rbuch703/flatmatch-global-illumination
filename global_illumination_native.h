#ifndef GLOBAL_ILLUMINATION_NATIVE
#define GLOBAL_ILLUMINATION_NATIVE

#ifdef __cplusplus
extern "C" {
#endif

#include "rectangle.h"

void performGlobalIlluminationNative(Geometry *geo, Vector3* lightColors, int numSamplesPerArea);


#ifdef __cplusplus
}
#endif

#endif
