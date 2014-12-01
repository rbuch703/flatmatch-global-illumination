
#include "geometry.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int max(int x, int y ) { return x > y ? x : y;}

Geometry* createGeometryObject() 
{
    return (Geometry*)malloc(sizeof(Geometry));
}

int geometryGetNumWalls(Geometry *geo) { return geo->numWalls;}

Rectangle* geometryGetWallPtr(Geometry *geo, int rectangleId)
{
    return &geo->walls[rectangleId];
}

Vector3*   geometryGetTexelPtr(Geometry *geo)
{
    return geo->texels;
}

void freeGeometry(Geometry *geo)
{
    free(geo->walls);
    free(geo->boxWalls);
    free(geo->lights);
    free(geo->windows);
    free(geo->texels);
    free(geo);
}

static int print( char* dst, int *dstPos, int dstSize, const char* fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);    
	
	int nChars = vsnprintf( dst ? &dst[*dstPos] : NULL, 
	                     dst? max(dstSize - *dstPos, 0) : 0, 
	                     fmt, argp);
    va_end(argp);
    
    *dstPos += nChars;
        
    return nChars;
}

static int printVector( char* dst, int *dstPos, int dstSize, Vector3 vec)
{
    return print(dst, dstPos, dstSize, "[%f, %f, %f]", vec.s[0], vec.s[1], vec.s[2]);
}


int writeJsonOutput( const Geometry *const geo, char* out, int outSize)
{
    #define PRINT( ...) print(out, &outPos, outSize, __VA_ARGS__)
    #define PRINTV(v)   printVector(out, &outPos, outSize, v);

    int outPos = 0;
    PRINT("{\n\"startingPosition\" : [%f, %f],\n", geo->startingPositionX, geo->startingPositionY);
    PRINT("\"layoutImageSize\" : [%d, %d],\n", geo->width, geo->height);
    PRINT("\"geometry\" : [\n");

    for ( int i = 0; i < geo->numWalls; i++)
    {
        
        PRINT("  { \"pos\": ");  PRINTV(geo->walls[i].pos);
        PRINT(", \"width\": ");  PRINTV(geo->walls[i].width);
        PRINT(", \"height\": "); PRINTV(geo->walls[i].height);
        PRINT(", \"textureId\": %d}", i);
        PRINT("%s\n", i+1 < geo->numWalls ? ",":"");
    }
    PRINT("],\n\"box\": [\n");

    for (int i = 0; i < geo->numBoxWalls; i++)
    {
        PRINT("  { \"pos\": "); PRINTV(geo->boxWalls[i].pos);
        PRINT(", \"width\": "); PRINTV(geo->boxWalls[i].width);
        PRINT(", \"height\": ");PRINTV(geo->boxWalls[i].height);
        PRINT("}%s\n", i+1 < geo->numBoxWalls ? ",":"");
    }
    PRINT("]\n}\n");

    #undef PRINT
    #undef PRINTV
    return outPos;
}

char* getJsonString(Geometry *geo)
{
    int nChars = writeJsonOutput(geo, NULL, 0);
    char* s = (char*)malloc(nChars+1);
    s[0] = '\0';
    writeJsonOutput(geo, s, nChars+1);
    return s;
}


