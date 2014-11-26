#ifndef GLOBAL_ILLUMINATION_NATIVE
#define GLOBAL_ILLUMINATION_NATIVE

#ifdef __cplusplus
extern "C" {
#endif

#include "rectangle.h"

void performPhotonMappingNative(Geometry geo, int numSamplesPerArea);
void performAmbientOcclusionNative(Geometry geo);
void performAmbientOcclusionNativeOnWall(Geometry *geo, Rectangle* wall);


#ifdef __cplusplus
}
#endif

#endif
