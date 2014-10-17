#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif


#include "vector3_cl.h"
//#include "color3.h"
#include "assert.h"

static const float TILE_SIZE = 10000/20.0f;   //lightmap texels per m²
static const int   SUPER_SAMPLING = 1;


/* Rectangle structure to be passed to OpenCL
 * - the alignment is required by OpenCL
 * - this struct should be as compact as possible to save vector registers on AMD OpenCL 
 */
typedef struct __attribute__ ((aligned(16))) Rectangle{
    Vector3 pos, width, height, n;
//    Vector3 color;
    cl_int3 lightmapSetup; // [0] = lightBaseIdx, [1] = tiles_width, [2] = tiles_height
//    int lightNumTiles;
//    int hNumTiles, vNumTiles;
//    float 
} Rectangle;

typedef struct Geometry {
    Rectangle *windows, *lights, *walls, *boxWalls;
    cl_int     numWindows, numLights, numWalls, numBoxWalls; 
    cl_int     width, height;
    float     startingPositionX, startingPositionY;
} Geometry;


Rectangle createRectangleV( const Vector3 _pos, const Vector3 _width, const Vector3 _height);
Rectangle createRectangle( float px, float py, float pz,
                           float wx, float wy, float wz,
                           float hx, float hy, float hz);

float intersects( const Rectangle *rect, Vector3 ray_src, Vector3 ray_dir, float closestDist);
int getNumTiles(const Rectangle *rect);
float getArea(const Rectangle *rect);
int getTileIdAt(const Rectangle *rect, const Vector3 p);

Vector3 getDiffuseColor(const Rectangle *rect, const Vector3 pos);
Vector3 getOrigin(const Rectangle *rect);
Vector3 getWidthVector(const Rectangle *rect);
Vector3 getHeightVector(const Rectangle *rect);
void saveAs(const Rectangle *rect, const char *filename, const Vector3 *lights);
void saveAsRaw(const Rectangle *rect, const char *filename, const Vector3 *lights);

#ifdef __cplusplus
}
#endif


#endif
