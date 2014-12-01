
#ifndef GEOMETRY_H
#define GEOMETRY_H 

#include "rectangle.h"

typedef struct Geometry {
    Rectangle  *windows, *lights, *walls, *boxWalls;
    cl_int     numWindows, numLights, numWalls, numBoxWalls; 
    cl_int     width, height;
    float      startingPositionX, startingPositionY;
    cl_int     numTexels;
    Vector3    *texels;

} Geometry;

void freeGeometry(Geometry *geo);

// the following methods are just for the emscripten/JavaScript interface
Geometry* createGeometryObject();
char* getJsonString(Geometry *geo);
int geometryGetNumWalls(Geometry *geo);
Rectangle* geometryGetWallPtr(Geometry *geo, int rectangleId);
Vector3*   geometryGetTexelPtr(Geometry *geo);



#endif
