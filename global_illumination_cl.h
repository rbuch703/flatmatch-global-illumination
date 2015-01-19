#ifndef GLOBAL_ILLUMINATION_CL
#define GLOBAL_ILLUMINATION_CL

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

void performGlobalIlluminationCl(Geometry *geo, int numSamplesPerArea);

#ifdef __cplusplus
}
#endif


#endif
