#ifndef GLOBAL_ILLUMINATION_CL
#define GLOBAL_ILLUMINATION_CL

#include "rectangle.h"

void performGlobalIlluminationCl(Geometry geo, 
                               Vector3* lightColors, cl_int numTexels,
                               int numSamplesPerArea);




#endif
