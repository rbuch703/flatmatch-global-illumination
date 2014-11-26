
#ifndef GEOSPHERE
#define GEOSPHERE

#include "vector3_cl.h"

/* Sets of almost equally-spaced points on the surface of a geodesic sphere. 
   Those points with z-value 0.0 (=vectors perpendicular to the surface normal)
   have been omitted.

   The different sets were create with different recursion depths and thus
   have different numbers of vertices (roughly a difference of factor 4 between
   successive sets)
*/

extern const Vector3 geoSphere2[];
extern const int geoSphere2NumVectors;

extern const Vector3 geoSphere3[];
extern const int geoSphere3NumVectors;

extern const Vector3 geoSphere4[];
extern const int geoSphere4NumVectors;

#endif

