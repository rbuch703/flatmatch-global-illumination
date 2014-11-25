#ifndef GLOBAL_ILLUMINATION_NATIVE
#define GLOBAL_ILLUMINATION_NATIVE

#ifdef __cplusplus
extern "C" {
#endif

#include "rectangle.h"

void performPhotonMappingNative(Geometry *geo, Vector3* lightColors, int numSamplesPerArea);
void performAmbientOcclusionNative(Geometry *geo, Vector3* lightColors);


#ifdef __cplusplus
}
#endif

#endif
