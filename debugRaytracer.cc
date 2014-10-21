
#include "png_helper.h" 
#include "math.h" // for INFINITY
#include "parseLayout.h"
#include <string.h> //for memset();
#include <map>
#include <iostream>

#include "rectangle.h"

/** begin of forward definitions from the photon mapper*/
extern "C" {
    typedef struct BspTreeNode {
        Rectangle plane;
        Rectangle *items;
        int       numItems;
        struct BspTreeNode *left;
        struct BspTreeNode *right;

    } BspTreeNode;

    void subdivideNode( BspTreeNode *node, int depth );
    int findClosestIntersection(Vector3 ray_pos, Vector3 ray_dir, const BspTreeNode *node, float *dist, float distShift, Rectangle **targetOut, int depth);
    void freeBspTree(BspTreeNode *root);
}
/** end of forward definitions from the photon mapper*/


using namespace std;

void freeGeometry(Geometry geo)
{
    free(geo.walls);
    free(geo.boxWalls);
    free(geo.lights);
    free(geo.windows);
}



int main()
{
    Geometry geo = parseLayout("137.png", /*scale*/1/30.0);
    cout << "Total number of walls: " << geo.numWalls << endl;

    int WIDTH = 1024;
    int HEIGHT= 768;
    uint8_t *imgData = new uint8_t[WIDTH*HEIGHT*sizeof(uint8_t)*3];
    memset(imgData, 0x20, WIDTH*HEIGHT*sizeof(uint8_t)*3);

    map<Rectangle*, uint32_t> colors;
    for ( int i = 0; i < geo.numWalls; i++)
    {
        int r = i % 5;
        int g = (i/5) % 5;
        int b = (i/25)% 5;
        //colors[ &geo.walls[i]] = b << 16 | g << 8 | r;
        geo.walls[i].lightmapSetup.s[0] = r*51;
        geo.walls[i].lightmapSetup.s[1] = g*51;
        geo.walls[i].lightmapSetup.s[2] = b*51;
        
    }
    
    
    BspTreeNode root;
    root.numItems = geo.numWalls;
    root.left = NULL;
    root.right= NULL;
    root.items =  (Rectangle*)malloc(sizeof(Rectangle) * geo.numWalls);
    memcpy( root.items, geo.walls, sizeof(Rectangle) * geo.numWalls);
    subdivideNode(&root, 0);
    
    int l = root.left ? root.left->numItems : 0;
    int r = root.right ? root.right->numItems : 0;
    cout << "node sizes (max/L/C/R):" << (( l > r ? l : r)+root.numItems) << ", " <<  l << ", " << root.numItems << ", " <<  r << endl;
    
    Vector3 camPos = createVector3(1, 1.2, 1.6);
    Vector3 camDir = normalized(createVector3(1, 1, 0));
    Vector3 screenCenter = add( camPos, camDir);
    float dx = 1/2000.0;
    float dy = 1/2000.0;
    
    Vector3 camUp  = createVector3(0, 0, 1);
    
    Vector3 camRight = cross(camDir, camUp );
    
    
    cout << "camRight: (" << camRight.s[0] << ", " << camRight.s[1] << ", " << camRight.s[2] << ")" << endl;
    //int y = 400;
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
    //for (int x = 200; x == 200; x++)
    {
        
        Vector3 screenPos = add3( screenCenter, mul(camRight, dx*(x- WIDTH/2)), mul(camUp, -dy*(y-HEIGHT/2))) ;
        Vector3 rayDir = normalized(sub(screenPos, camPos));

        //cout << "rayDir: (" << rayDir.s[0] << ", " << rayDir.s[1] << ", " << rayDir.s[2] << ")" << endl;
        
        float dist= INFINITY;
        Rectangle *targetOut = NULL;
#if 0        
        for ( int i = 0; i < geo.numWalls; i++)
        {
            //cout << "wall pos: " << geo.walls[i].pos.s[0] << ", " << geo.walls[i].pos.s[1] << ", " << geo.walls[i].pos.s[2]  << endl;
            Rectangle *target = &(geo.walls[i]);
            float dist_new = intersects(target , camPos, rayDir, dist);
            if (dist_new < 0)
                continue;
                
            if (dist_new < dist) {
                targetOut = target;
                dist = dist_new;
            }
        }
#endif
        //exit(0);
        if (!findClosestIntersection(camPos, rayDir, &root, &dist, 0, &targetOut, 0) )
            continue;

        if (!targetOut)
            continue;
            
        //cout << "hit target " << targetOut << endl;
        cl_int3 col = targetOut->lightmapSetup;
        uint8_t r = col.s[0];
        uint8_t g = col.s[1];
        uint8_t b = col.s[2];
        //cout << (int)r << ", " << (int)g << ", " << (int)b << endl;
        //cout << (int)r << "/" << (int)g << "/" << (int)b << endl;
        imgData[ (y*WIDTH+x)*3 + 0] = r;
        imgData[ (y*WIDTH+x)*3 + 1] = g;
        imgData[ (y*WIDTH+x)*3 + 2] = b;
          

    }

    write_png_file("image.png", WIDTH, HEIGHT, PNG_COLOR_TYPE_RGB, imgData);
    delete [] imgData;
        
    freeBspTree(&root);
    freeGeometry(geo);
    return 0;
}    

