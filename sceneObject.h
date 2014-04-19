#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif


#include "vector3_cl.h"
//#include "color3.h"
#include <math.h>
#include "assert.h"


static const float TILE_SIZE = 4;

typedef struct /*__attribute__ ((aligned(16))) */Rectangle{

    Vector3 pos, width, height, n;
    Vector3 width_norm, height_norm;
    Vector3 width_per_tile, height_per_tile;
    Vector3 color;
    float hLength, vLength;
    
    int hNumTiles, vNumTiles;
    int lightBaseIdx;

} Rectangle;


Rectangle createRectangle( const Vector3 _pos, const Vector3 _width, const Vector3 _height);
Rectangle createRectangleWithColor( const Vector3 _pos, const Vector3 _width, const Vector3 _height, const Vector3 col);
float intersects( const Rectangle *rect, Vector3 ray_src, Vector3 ray_dir, float closestDist);
//Vector3 getNormalAt(const Rectangle *rect, const Vector3&); 
int getNumTiles(const Rectangle *rect);
int getTileIdAt(const Rectangle *rect, const Vector3 p);
//Vector3 getTileCenter(const Rectangle &rect, int id);
Vector3 getDiffuseColor(const Rectangle *rect, const Vector3 pos);
//void setTileColor(const int tileId, const Color3 &color);
//Tile& getTileAt(Rectangle *rect, const Vector3 &pos);
//Tile& getTile(Rectangle *rect, const int tileId);
Vector3 getOrigin(const Rectangle *rect);
Vector3 getWidthVector(const Rectangle *rect);
Vector3 getHeightVector(const Rectangle *rect);
void saveAs(const Rectangle *rect, const char *filename, Vector3 *lights);

#ifdef __cplusplus
}
#endif


#endif
