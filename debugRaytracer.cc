
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

int findClosestIntersectionLinear(Vector3 rayPos, Vector3 rayDir, Rectangle *rects, int numRects, float* dist, Rectangle **targetOut)
{
    *dist = INFINITY;
    for ( int i = 0; i < numRects; i++)
    {
        //cout << "wall pos: " << geo.walls[i].pos.s[0] << ", " << geo.walls[i].pos.s[1] << ", " << geo.walls[i].pos.s[2]  << endl;
        Rectangle *target = &(rects[i]);
        float dist_new = intersects(target , rayPos, rayDir, *dist);
        if (dist_new < 0)
            continue;
            
        if (dist_new < *dist) {
            if (targetOut)
                *targetOut = target;
            *dist = dist_new;
        }
    }
    
    return (*dist) != INFINITY;

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

int main()
{
    Image* img = loadImage("137.png");
    Geometry *geo = parseLayout(img, /*scale*/1/30.0, 500);
    colorRects(geo->walls, geo->numWalls);
    cout << "Total number of walls: " << geo->numWalls << endl;
    freeImage(img);

    img = createImage( 100, 75);
    
    //BspTreeNode *root = buildBspTree(geo->walls, geo->numWalls);
    
    Vector3 camPos = createVector3(1, 1.2, 1.6);
    Vector3 camDir = normalized(createVector3(1, 1, 0));
    Vector3 screenCenter = add( camPos, camDir);
    Vector3 camUp  = createVector3(0, 0, 1);
    Vector3 camRight = cross(camDir, camUp );
    //cout << "camRight: (" << camRight.s[0] << ", " << camRight.s[1] << ", " << camRight.s[2] << ")" << endl;
    
    const float dx = 1/2000.0;
    const float dy = 1/2000.0;
    
    //int y = 400;
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
    //for (int x = 200; x == 200; x++)
    {
        Vector3 screenPos = add3( screenCenter, mul(camRight, dx*(x- img->width/2)), mul(camUp, -dy*(y-img->height/2))) ;
        Vector3 rayDir = normalized(sub(screenPos, camPos));
        //cout << "rayDir: (" << rayDir.s[0] << ", " << rayDir.s[1] << ", " << rayDir.s[2] << ")" << endl;
        
        float dist= INFINITY;
        Rectangle *targetOut = NULL;

/*        if (!findClosestIntersection(camPos, rayDir, root, &dist, 0, &targetOut, 0) )*/
        if (!findClosestIntersectionLinear(camPos, rayDir, geo->walls, geo->numWalls, &dist, &targetOut) )
            continue;

        if (!targetOut)
            continue;
            
        setImagePixel(img, x, y, toRgba32(targetOut->lightmapSetup));
    }

    saveImageAs(img, "image.png");
    freeImage(img);
        
    //freeBspTree(root);
    freeGeometry(geo);
    return 0;
}    

