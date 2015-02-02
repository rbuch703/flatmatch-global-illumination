
#include <math.h> // for INFINITY
#include <string.h> //for memset();
#include <assert.h>
#include <map>
#include <iostream>

#include "rectangle.h"
#include "image.h"
#include "parseLayout.h"
#include "png_helper.h" 

/** begin of forward declarations from photonmap.c*/
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
/** end of forward declarations from photonmap.c*/

/** begin of forward declarations from radiosityNative.c **/
extern "C" {

    typedef struct RectInfo {
        Rectangle rect;
        float     minDistance;
    } RectInfo;

    Rectangle* findClosestIntersectionSorted(Vector3 rayPos, Vector3 rayDir, RectInfo *rects, int numRects, float* dist);
    int getSortedIntersectableRects(const Geometry *geo, Vector3 camPos, Vector3 camDir, RectInfo** rects);
}
/** end of forward declarations from radiosityNative.c **/

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
            
        if (dist_new <= *dist) {
                hitObj = target;
            *dist = dist_new;
        }
    }
    
    return hitObj;

}



/*
typedef struct BoundingBox2D {
    float minX, maxX, minY, maxY;
    cl_int3 lightmapSetup; // [0] = lightBaseIdx, [1] = tiles_width, [2] = tiles_height
} 2DRectangle;

BoundingBox2D toHorizontalBoundingBox(const Rectangle *rect)
{
    assert( fabs(rect->n.s[0]) < 1E-5 && fabs(rect->n.s[1]) < 1E-5);
//    assert( fabs(rect->width
}*/


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



int main()
{
    Image* img = loadImage("137.png");
    Geometry *geo = parseLayout(img, /*scale*/1/30.0, 500);
    colorRects(geo->walls, geo->numWalls);
    cout << "[INF] #walls: " << geo->numWalls << endl;
    freeImage(img);
    img = createImage( 4096, 3072);    //output image
//    img = createImage( 1024, 768);    //output image
    
    
    //BspTreeNode *root = buildBspTree(geo->walls, geo->numWalls);

    Vector3 camPos = createVector3(1+4, 1.2+4, 1.6);
    Vector3 camDir = normalized(createVector3(1, 1, 0));
    Vector3 screenCenter = add( camPos, camDir);
    Vector3 camUp  = createVector3(0, 0, 1);
    Vector3 camRight = cross(camDir, camUp );

    /*
    Vector3 camPos = createVector3(10, 7, 20);
    Vector3 camDir = normalized(createVector3(0, 0, -1));
    Vector3 screenCenter = add( camPos, camDir);
    Vector3 camUp  = createVector3(0, 1, 0);
    Vector3 camRight = cross(camDir, camUp );*/
    //cout << "camRight: (" << camRight.s[0] << ", " << camRight.s[1] << ", " << camRight.s[2] << ")" << endl;



/*    for (int i = 0; i < numRects; i++)
        printf("%d\t %f - %d, %d, %d\n", i, rects[i].minDistance,
            rects[i].rect.lightmapSetup.s[0], rects[i].rect.lightmapSetup.s[1],  rects[i].rect.lightmapSetup.s[2]);*/

    RectInfo *rects;
    int numRects =      getSortedIntersectableRects(geo, camPos, camDir, &rects);
    cout << "[INF] #intersection candidates: " << numRects << endl;
    //cout << "floor/ceiling rects: " << arFloorCeiling.numItems << endl;
    
    const float dx = 1/1000.0;
    const float dy = 1/1000.0;
    
    /*int y = 361;
    for (int x = 47; x <= 60; x++)*/
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
    {
        Vector3 screenPos = add3( screenCenter, mul(camRight, dx*(x- img->width/2)), mul(camUp, -dy*(y-img->height/2))) ;
        Vector3 rayDir = normalized(sub(screenPos, camPos));
        assert(dot(rayDir, camDir) >= 0);
        
        /* As the rays start always at a height between the floor and the 
           ceiling, they can at most hit one of them: If the ray faces down,
           it can only hit hte floor; otherwise it can only hit the ceiling 
        */
        /*Rectangle       horizontalPlane      = (rayDir.s[2] < 0) ? floorPlane : ceilingPlane;
        RectangleArray  horizontalPlaneRects = (rayDir.s[2] < 0) ? floorRects : ceilingRects;
        float horizontalPlaneDistance = rayDir.s[2] == 0 ? INFINITY :
                                        distanceOfIntersectionWithRectPlane(camPos, rayDir, &horizontalPlane);*/

        float dist = INFINITY;//horizontalPlaneDistance;
        assert(dist > 0);
        Rectangle *hitObj = findClosestIntersectionSorted(camPos, rayDir, rects,numRects, &dist);

        //Rectangle* hitObj = findClosestIntersectionLinear(camPos, rayDir, geo->walls, geo->numWalls, &dist);

        if (!hitObj)
            continue;
            
        setImagePixel(img, x, y, toRgba32(hitObj->lightmapSetup));

        /*dist *= 1.00001;
        for (int i = 0; i < horizontalPlaneRects.numItems; i++)
        {
            float hitDist = intersects( &horizontalPlaneRects.data[i], camPos, rayDir, dist);
            if (hitDist < dist)
            {
                hitObj = &horizontalPlaneRects.data[i];
                dist = hitDist;
            }
        }

        if (hitObj)
            setImagePixel(img, x, y, toRgba32(hitObj->lightmapSetup));*/
    }

    saveImageAs(img, "image.png");
    free(rects);
    freeImage(img);
        
    //freeBspTree(root);
    freeGeometry(geo);
    return 0;
}    

