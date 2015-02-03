
#include <string.h> //for memcpy
#include <stdio.h>  //for printf
#include <assert.h>
#include <math.h>

#include "radiosityNative.h"
#include "vector3_cl.h"
#include "rectangle.h"

typedef struct RectInfo {
    Rectangle rect;
    float     minDistance;
} RectInfo;


int compareRectInfo( const void* a, const void* b) 
{ 
    float d1 = ((RectInfo*)a)->minDistance;
    float d2 = ((RectInfo*)b)->minDistance;
//    printf("## %f, %f\n", (, ((RectInfo*)b)->minDistance);
    return (d1 < d2) ? -1 : ((d1 > d2) ? 1 : 0);
}

int getSortedIntersectableRects(Rectangle* rectsIn, int numRectsIn, Vector3 camPos, Vector3 camDir, RectInfo** rectsOut)
{
    RectangleArray potentialHits= initRectangleArray();
/*    RectangleArray floorRects = initRectangleArray();
    RectangleArray ceilingRects = initRectangleArray();
    Rectangle floorPlane   = createRectangleV( vec3( 0, 0, 0),   vec3( 0, 1, 0), vec3( 1, 0, 0), 500);
    Rectangle ceilingPlane = createRectangleV( vec3( 0, 0, 2.6), vec3( 1, 0, 0), vec3( 0, 1, 0), 500);*/

    for (int i = 0; i < numRectsIn; i++)
    {
        Rectangle *wall = &(rectsIn[i]);
        
        if ( dot(wall->n, sub(wall->pos, camPos)) > 0)         continue; // backface --> cannot be hit
        if (isBehindRay(wall, camPos, camDir)) continue; // behind the camera --> cannot be hit
/*
        //normal is pointing straight up or straight down --> is a horizontal rect
        if (fabs(wall->n.s[0]) < 1E-5 && fabs(wall->n.s[1]) < 1E-5)
        {
            if (fabs(wall->pos.s[2]) < 1E-6) // is a floor rect
            { insertIntoRectangleArray(&floorRects, *wall); continue;}
            
            if (fabs(wall->pos.s[2] - 2.60) < 1E-5)
            { insertIntoRectangleArray(&ceilingRects, *wall); continue;}
        }*/

        insertIntoRectangleArray(&potentialHits, *wall);
    }
    
    int numRectsOut = potentialHits.numItems;
    *rectsOut = (RectInfo*)malloc( sizeof(RectInfo) * numRectsOut);
    for (int i = 0; i < numRectsOut; i++)
        (*rectsOut)[i] = (RectInfo){.rect = potentialHits.data[i], 
                    .minDistance = getShortestDistanceRectToPoint(&potentialHits.data[i], camPos)};
    qsort((*rectsOut), numRectsOut, sizeof(RectInfo), compareRectInfo);

    freeRectangleArray(&potentialHits);
    return numRectsOut;
}

/* find closest intersection of 'ray' with any of the 'rects', where the rects
   are sorted by their minimum distance to the ray source. 'dist' is the 
   distance to the closest object hit yet, and is updated as new intersections
   are found*/
Rectangle* findClosestIntersectionSorted(Vector3 rayPos, Vector3 rayDir, RectInfo *rects, int numRects, float* dist)
{
    Rectangle* hitObj = NULL;
    for ( int i = 0; i < numRects; i++)
    {
        //cout << "wall pos: " << geo.walls[i].pos.s[0] << ", " << geo.walls[i].pos.s[1] << ", " << geo.walls[i].pos.s[2]  << endl;
        RectInfo *target = &(rects[i]);
        if (*dist < target->minDistance)
            return hitObj;

        float dist_new = intersects(&target->rect, rayPos, rayDir, *dist);
        if (dist_new < 0)
            continue;
            
        if (dist_new <= *dist) {
                hitObj = &target->rect;
            *dist = dist_new;
        }
    }
    
    return hitObj;

}



void performRadiosityNative(Geometry *geo)
{
/*    for (int i = 0; i < 1000; i++)
    {
        Vector3 dir = getCosineDistributedRandomRay( vec3(0,0,1));
        printf("%f, %f, %f\n", dir.s[0], dir.s[1], dir.s[2]);
    }
    exit(0);*/


    static const float reflectance = 0.3;

    // create an array of all rectangles, including windows and lighting
    int numRects = geo->numWalls + geo->numWindows + geo->numLights;
    Rectangle* rects = malloc( numRects * sizeof(Rectangle));
    memcpy(rects,                 geo->walls,   geo->numWalls *   sizeof(Rectangle));
    memcpy(&rects[geo->numWalls], geo->windows, geo->numWindows * sizeof(Rectangle));
    memcpy(&rects[geo->numWalls+geo->numWindows], geo->lights, geo->numLights * sizeof(Rectangle));

    /* add lightmap indices for windows and lights, which did not get them on
       creation as they are not necessary for the other GI algorithms. */
    int numTexels = geo->numTexels;
    for (int i = geo->numWalls; i < geo->numWalls + geo->numWindows; i++)
    {
        rects[i].lightmapSetup.s[0] = numTexels;
        numTexels += getNumTiles(&rects[i]);
    }
    int firstWindowTexel = geo->numTexels;
    int firstLightTexel  = numTexels;

    for (int i = geo->numWalls+ geo->numWindows; i < geo->numWalls + geo->numWindows + geo->numLights; i++)
    {
        rects[i].lightmapSetup.s[0] = numTexels;
        numTexels += getNumTiles(&rects[i]);
    }

   
    printf("%d/%d texels\n", geo->numTexels, numTexels);
    
    Vector3 *srcTexels = malloc( numTexels * sizeof(Vector3));
    Vector3 *destTexels = malloc( numTexels * sizeof(Vector3));
    
    
    for (int i = 0; i < numTexels; i++)
    {
        if (i < firstWindowTexel) //is a wall texel
            srcTexels[i] = vec3(0,0,0);
        else if (i < firstLightTexel) //is a window texel
            srcTexels[i] = vec3(30,30,30);
        else
            srcTexels[i] = vec3(28,28,32);
        
        destTexels[i] = vec3(0,0,0);
    }

    int geoSphereNumVectors = 10000;
    int32_t **sourceTexelIds = malloc(numTexels * sizeof(int32_t*));
    for (int i = 0; i < numTexels; i++)
    {
        sourceTexelIds[i] = malloc( geoSphereNumVectors * sizeof(int32_t));
        for (int j = 0; j < geoSphereNumVectors; j++)
            sourceTexelIds[i][j] = -1;
    }
            
    printf("[INF] Determining form factors.\n");
    printf("\n\e[1A\e7");
    //while(1);
    
    for (int i = 0; i < geo->numWalls; i++)
    {
        if ( (i+1)% 10 == 0)
            printf("\e8[INF] processing wall %d/%d\e[K\n", i+1, geo->numWalls);
        Rectangle* wall = &geo->walls[i];

        /*Vector3 b1, b2;
        createBase( wall->n, &b1, &b2);*/
        

        for (int j = 0; j < getNumTiles(wall); j++)
        {
            /*printf("[DBG] tile pos: (%f, %f, %f), normal: (%f, %f, %f)\n", 
                    wall->pos.s[0], wall->pos.s[1], wall->pos.s[2],
                    wall->n.s[0], wall->n.s[1], wall->n.s[2]);*/
            int texelIdx = wall->lightmapSetup.s[0] + j;

            RectInfo *rectInfos;    
            int numRectInfos = getSortedIntersectableRects(rects, numRects, getTileCenter(wall, j), wall->n, &rectInfos);
            //printf("got list of %d rects\n", numRectInfos);
            //printf("normal: %f, %f, %f\n", wall->n.s[0], wall->n.s[1], wall->n.s[2]);

            for (int k = 0; k < geoSphereNumVectors; k++)
            {
                Vector3 dir = getCosineDistributedRandomRay(wall->n);
                //float fac = dot(dir, wall->n);

                assert(fabsf(length(dir) - 1.0f) < 1E-6);
                
                Vector3 pos = getTileCenter(wall, j);
                /*printf("[DBG] pos: (%f, %f, %f), dir: (%f, %f, %f)\n",
                       pos.s[0], pos.s[1], pos.s[2], dir.s[0], dir.s[1], dir.s[2]);*/
                pos = add(pos, mul(dir, 1E-5));

                float dist = INFINITY;
                Rectangle* target = findClosestIntersectionSorted(pos, dir, rectInfos, numRectInfos, &dist);
                //printf("[DBG] target: %p\n", target);
                assert(target);
                if (!target)
                    continue;
                    
                //float fac = -dot(target->n, dir);
                //assert(fac >= 0);
                   
                Vector3 hitPos = add (pos, mul(dir, dist));
                int srcTexelId = target->lightmapSetup.s[0] + getTileIdAt( target, hitPos);

                // "the j'th source for light incoming to 'texelId' is 'srcTexelId' "
                sourceTexelIds[texelIdx][k] = srcTexelId;

                //printf("target is %d\n", srcTexelId);
                //printf("%d\n", srcTexelId);
                //if (target->lightmapSetup.s[0] > geo->numTexels)
                //    printf("#\n");

            }
            
            free(rectInfos);
        }
    }
    printf("[INF] done; now distributing radiosity\n");

    for (int depth = 0; depth < 1/*12*/; depth++)
    {
        for (int destTexelId = 0; destTexelId < numTexels; destTexelId++)
            for (int j = 0; j < geoSphereNumVectors; j++)
            {
                int srcTexelId = sourceTexelIds[destTexelId][j];
                if (srcTexelId < 0)
                    continue;
                    
                inc( &destTexels[destTexelId], srcTexels[srcTexelId]);
            }

        for (int i = 0; i < numTexels; i++)
        {
            srcTexels[i] = add( mul(srcTexels[i], 1-reflectance), 
                                mul(destTexels[i], reflectance/geoSphereNumVectors));
            destTexels[i] = vec3(0,0,0);
        }
    }
    
    // copy back only those texels corresponding to the walls (omitted window texels)
    for (int i = 0; i < geo->numTexels; i++)
        geo->texels[i] = srcTexels[i];
        
    free(rects);
    free(srcTexels);
    free(destTexels);

    for (int i = 0; i < numTexels; i++)
        free(sourceTexelIds[i]);

    free(sourceTexelIds);   
    //freeBspTree(root);
//    free(root);

}

