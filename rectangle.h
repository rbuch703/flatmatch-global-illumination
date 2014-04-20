#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif


#include "vector3_cl.h"
//#include "color3.h"
#include <math.h>
#include "assert.h"


static const float TILE_SIZE = 10;

typedef struct __attribute__ ((aligned(16))) Rectangle{
    Vector3 pos, width, height, n;
    Vector3 color;
    int lightBaseIdx;
} Rectangle;


Rectangle createRectangle( const Vector3 _pos, const Vector3 _width, const Vector3 _height);
Rectangle createRectangleWithColor( const Vector3 _pos, const Vector3 _width, const Vector3 _height, const Vector3 col);
float intersects( const Rectangle *rect, Vector3 ray_src, Vector3 ray_dir, float closestDist);
int getNumTiles(const Rectangle *rect);
int getTileIdAt(const Rectangle *rect, const Vector3 p);
Vector3 getDiffuseColor(const Rectangle *rect, const Vector3 pos);
Vector3 getOrigin(const Rectangle *rect);
Vector3 getWidthVector(const Rectangle *rect);
Vector3 getHeightVector(const Rectangle *rect);
void saveAs(const Rectangle *rect, const char *filename, Vector3 *lights);

#ifdef __cplusplus
}
#endif


#endif
