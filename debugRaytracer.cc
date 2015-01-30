
#include <math.h> // for INFINITY
#include <string.h> //for memset();
#include <map>
#include <iostream>

#include "rectangle.h"
#include "image.h"
#include "parseLayout.h"
#include "png_helper.h" 

/** begin of forward definitions from the photon mapper*/
extern "C" {
    typedef struct BspTreeNode {
        Rectangle plane;
        Rectangle *items;
        int       numItems;
        struct BspTreeNode *left;
        struct BspTreeNode *right;

    } BspTreeNode;

    BspTreeNode* buildBspTree( Rectangle* items, int numItems);  
    int findClosestIntersection(Vector3 ray_pos, Vector3 ray_dir, const BspTreeNode *node, float *dist, float distShift, Rectangle **targetOut, int depth);
    void freeBspTree(BspTreeNode *root);
    float intersects( const Rectangle *rect, const Vector3 ray_src, const Vector3 ray_dir, const float closestDist);

}
/** end of forward definitions from the photon mapper*/

using namespace std;

Rectangle* findClosestIntersectionLinear(Vector3 rayPos, Vector3 rayDir, Rectangle *rects, int numRects, float* dist)
{
    Rectangle* hitObj = NULL;
    for ( int i = 0; i < numRects; i++)
    {
        //cout << "wall pos: " << geo.walls[i].pos.s[0] << ", " << geo.walls[i].pos.s[1] << ", " << geo.walls[i].pos.s[2]  << endl;
        Rectangle *target = &(rects[i]);
        float dist_new = intersects(target , rayPos, rayDir, *dist);
        if (dist_new < 0)
            continue;
            
        if (dist_new < *dist) {
                hitObj = target;
            *dist = dist_new;
        }
    }
    
    return hitObj;

}


typedef struct RectInfo {
    Rectangle rect;
    float     minDistance;
} RectInfo;


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
            
        if (dist_new < *dist) {
                hitObj = &target->rect;
            *dist = dist_new;
        }
    }
    
    return hitObj;

}



void colorRects( Rectangle* rects, int numRects)
{
    //map<Rectangle*, uint32_t> colors;
    for ( int i = 0; i < numRects; i++)
    {
        int r = i % 5;
        int g = (i/5) % 5;
        int b = (i/25)% 5;
        //colors[ &geo.walls[i]] = b << 16 | g << 8 | r;
        rects[i].lightmapSetup.s[0] = r*51;
        rects[i].lightmapSetup.s[1] = g*51;
        rects[i].lightmapSetup.s[2] = b*51;
    }
}

uint32_t toRgba32(cl_int3 col) { return 0xFF000000 | (col.s[2] << 16) | (col.s[1] << 8) | col.s[0]; }

ostream &operator <<(ostream &os, const Vector3 &v)
{
    os << "(" << v.s[0] << ", " << v.s[1] << ", " << v.s[2] << ")";
    return os;
}


int compareRectInfo( const void* a, const void* b) 
{ 
    float d1 = ((RectInfo*)a)->minDistance;
    float d2 = ((RectInfo*)b)->minDistance;
//    printf("## %f, %f\n", (, ((RectInfo*)b)->minDistance);
    return (d1 < d2) ? -1 : ((d1 > d2) ? 1 : 0);
}

int main()
{
    /*
    Rectangle r = createRectangleV( vec3(0,0,0), vec3(1,0,1), vec3(0,1,1), 500);
    Vector3 p = vec3( 2,3,1);
    float res = getShortestDistanceRectToPoint( &r, p);
    cout << "distance is " << res << endl;
    exit(0);*/

    Image* img = loadImage("137.png");
    Geometry *geo = parseLayout(img, /*scale*/1/30.0, 500);
    colorRects(geo->walls, geo->numWalls);
    cout << "Total number of walls: " << geo->numWalls << endl;
    freeImage(img);
//    img = createImage( 4096, 3072);    //output image
    img = createImage( 1024, 768);    //output image
    
    
    //BspTreeNode *root = buildBspTree(geo->walls, geo->numWalls);
    /*
    Vector3 camPos = createVector3(1+4, 1.2+4, 1.6);
    Vector3 camDir = normalized(createVector3(1, 1, 0));
    Vector3 screenCenter = add( camPos, camDir);
    Vector3 camUp  = createVector3(0, 0, 1);
    Vector3 camRight = cross(camDir, camUp );*/

    
    Vector3 camPos = createVector3(10, 7, 20);
    Vector3 camDir = normalized(createVector3(0, 0, -1));
    Vector3 screenCenter = add( camPos, camDir);
    Vector3 camUp  = createVector3(0, 1, 0);
    Vector3 camRight = cross(camDir, camUp );
    //cout << "camRight: (" << camRight.s[0] << ", " << camRight.s[1] << ", " << camRight.s[2] << ")" << endl;


    RectangleArray arFloorCeiling = initRectangleArray();
    RectangleArray potentialHits= initRectangleArray();
/*    Rectangle ceilingPlane = createRectangleV( 
        vec3( 0, 0, 2.6), vec3( 1, 0, 0), vec3( 0, 1, 0), 500);

    Rectangle floorPlane = createRectangleV( 
        vec3( 0, 0, 0), vec3( 0, 1, 0), vec3( 1, 0, 0), 500);*/

    //int numSkipableWalls = 0;
    for (int i = 0; i < geo->numWalls; i++)
    {
        Rectangle *wall = &(geo->walls[i]);
        
        if ( dot(wall->n, camDir) > 0)         continue; // backface --> cannot be hit
        if (isBehindRay(wall, camPos, camDir)) continue; // behind the camera --> cannot be hit

         //is a ceiling or floor rect, if its height is at 0.0 or 2.6m and its normal is pointing
         //straight up or straight down
/*        int isFloorOrCeiling = fabs(wall->n.s[0]) < 1E-5 && fabs(wall->n.s[1]) < 1E-5 &&
                              (fabs(wall->pos.s[2]) < 1E-6 || fabs(wall->pos.s[2] - 2.60) < 1E-5);*/

        //insertIntoRectangleArray(isFloorOrCeiling ? &arFloorCeiling : &potentialHits, *wall);
        insertIntoRectangleArray(&potentialHits, *wall);

        //cout << wall->pos  << "\t" << wall->n << endl;
        
        //wall->lightmapSetup = { .s = {255, 255, 255} };
        //numSkipableWalls += 1;
    }
    
    int numRects = potentialHits.numItems;
    RectInfo* rects = (RectInfo*)malloc( sizeof(RectInfo) * numRects);
    for (int i = 0; i < numRects; i++)
        rects[i] = {.rect = potentialHits.data[i], 
                    .minDistance = getShortestDistanceRectToPoint(&potentialHits.data[i], camPos)};
    qsort(rects, numRects, sizeof(RectInfo), compareRectInfo);
/*    for (int i = 0; i < numRects; i++)
        for (int j = i+1; j < numRects; j++)
        {
            if (compareRectInfo( &rects[i], &rects[j]) > 0)
            {
                RectInfo tmp = rects[i];
                rects[i] = rects[j];
                rects[j] = tmp;
            }
        }*/
    for (int i = 0; i < numRects; i++)
        printf("%d\t %f\n", i, rects[i].minDistance);
    
    
    cout << "potential hits: " << potentialHits.numItems << endl;
    cout << "floor/ceiling rects: " << arFloorCeiling.numItems << endl;

    
    const float dx = 1/1000.0;
    const float dy = 1/1000.0;
    
/*    int y = 281;
    for (int x = 660; x <= 690; x++)*/
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
    {
        Vector3 screenPos = add3( screenCenter, mul(camRight, dx*(x- img->width/2)), mul(camUp, -dy*(y-img->height/2))) ;
        Vector3 rayDir = normalized(sub(screenPos, camPos));
        
        float ceilingOrFloorDistance = INFINITY;
        /*if (rayDir.s[2] < 0)
            ceilingOrFloorDistance = distanceOfIntersectionWithPlane(camPos, rayDir, floorPlane.n, floorPlane.pos);
        else if (rayDir.s[2] > 0)
            ceilingOrFloorDistance = distanceOfIntersectionWithPlane(camPos, rayDir, ceilingPlane.n, ceilingPlane.pos);*/
            
        //cout << "rayDir: (" << rayDir.s[0] << ", " << rayDir.s[1] << ", " << rayDir.s[2] << ")" << endl;
        //printf("floor/ceiling distance is %f\n", ceilingOrFloorDistance);
        float dist= ceilingOrFloorDistance;
        assert(dist > 0);
        Rectangle *hitObj = NULL;

/*        if (!findClosestIntersection(camPos, rayDir, root, &dist, 0, &targetOut, 0) )*/
//        if (!findClosestIntersectionLinear(camPos, rayDir, geo->walls, geo->numWalls, &dist, &targetOut) )
        if (! (hitObj = findClosestIntersectionSorted(camPos, rayDir, rects,numRects, &dist)) )
//        if (! (hitObj = findClosestIntersectionLinear(camPos, rayDir, potentialHits.data, potentialHits.numItems, &dist)) )
            continue;

        setImagePixel(img, x, y, toRgba32(hitObj->lightmapSetup));
    }

    saveImageAs(img, "image.png");
    freeImage(img);
        
    //freeBspTree(root);
    freeGeometry(geo);
    return 0;
}    

