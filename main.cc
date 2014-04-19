
#include "vector3_sse.h"
#include "sceneObject.h"
#include "parseLayout.h"
#include "png_helper.h"

#include <list>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <stdlib.h> //for rand()
#include <string>
#include <string.h> //for memset
using namespace std;


Rectangle* objects;
int numObjects;

Vector3* lightColors;
int numLightColors;

Vector3 getUniformDistributedRandomRay(Vector3 ndir, Vector3 udir, Vector3 vdir)
{
/*    //Marsaglia polar method for creating gaussian-distributed random variates
    float x = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    float y = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    float s = x*x+y*y;

    float u = x * sqrt( (-2*log(s)/s)); //has to be the natural logarithm! ( c's log() is fine)
    float v = y * sqrt( (-2*log(s)/s));

    x = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    y = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    s = x*x+y*y;
    float n = x * sqrt( (-2*log(s)/s));    

    float len = sqrt(u*u+v*v+n*n);
    u/=len;
    v/=len;
    n/=len;*/
    
    //HACK: computes not a spherically uniform distribution, but a lambertian quarter-sphere (lower half of hemisphere)
    //# Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand() / (float)RAND_MAX);
    float phi = 2 * 3.141592 * rand() / (float)RAND_MAX;

    //# Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    if (v > 0)  //project to lower quadsphere (no light from below the horizon)
        v = -v;

    //# Convert to a direction on the hemisphere defined by the normal
    return add(add(mul(udir, u), mul(vdir, v)), mul(ndir, n));

}

Vector3 getCosineDistributedRandomRay(Vector3 ndir, Vector3 udir, Vector3 vdir) {
    //# Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand() / (float)RAND_MAX);
    float phi = 2 * 3.141592 * rand() / (float)RAND_MAX;

    //# Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    //# Convert to a direction on the hemisphere defined by the normal
    //return udir*u + vdir*v + ndir*n;
    return add(add(mul(udir, u), mul(vdir, v)), mul(ndir, n));
    
}

//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void createBase(Vector3 ndir, Vector3 &c1, Vector3 &c2) {
    c1 = createVector3(0,0,1);
    if (fabs(dot(c1, ndir)) == 1) //are colinear --> cannot build coordinate base
        c1 = createVector3(0,1,0);
        
    c2 = normalized( cross(c1,ndir));
    c1 = normalized( cross(c2,ndir));
}

Rectangle* getClosestObject(const Vector3 &ray_src, const Vector3 &ray_dir, Rectangle *objects, int numObjects, float &dist_out)
{
    Rectangle *closestObject = NULL;
    dist_out = INFINITY;
    
    //int numObjects = objects.size();
    
    for ( int i = 0; i < numObjects; i++)
    {
        float dist = intersects( &(objects[i]), ray_src, ray_dir, dist_out);
        if (dist < 0)
            continue;
            
        if (dist < dist_out) {
            closestObject = &(objects[i]);
            dist_out = dist;
        }
    }
    return closestObject;
}

Vector3 getObjectColorAt(Rectangle* rect, Vector3 pos)
{
    
    Vector3 light = lightColors[ rect->lightBaseIdx + getTileIdAt(rect, pos)];
    return createVector3( light[0] * rect->color[0],
                          light[1] * rect->color[1],
                          light[2] * rect->color[2]);
}

Vector3 getColor(Vector3 ray_src, Vector3 ray_dir, Rectangle* objects, int numObjects/*, int depth = 0*/)
{

    float hitDist;
    Rectangle *hitObject = getClosestObject(ray_src, ray_dir, objects, numObjects, hitDist);

    if (! hitObject)         
        return createVector3(0,0,0);

    //Vector3 intersect_pos = ray_src + ray_dir * hitDist;
    Vector3 intersect_pos = add(ray_src, mul(ray_dir, hitDist));
    return getObjectColorAt(hitObject, intersect_pos);
}

int main()
{
    static const Vector3 wallColor = createVector3(0.8, 0.8, 0.8);
    
    vector<Rectangle> rects = parseLayout("layout.png");
    numObjects = 2 + rects.size();
    objects = (Rectangle*)malloc( numObjects * sizeof(Rectangle));
    objects[0] = createRectangleWithColor( createVector3(0,0,0), createVector3(0, 1000, 0), createVector3(1000, 0, 0), wallColor);    // floor
    objects[1] = createRectangleWithColor( createVector3(0,0,200), createVector3(1000, 0, 0), createVector3(0, 1000, 0), wallColor);  // ceiling

    int idx = 2;    
    for ( vector<Rectangle>::const_iterator it = rects.begin(); it != rects.end(); it++)
        objects[idx++] = *it;

    numLightColors = 0;
    for ( int i = 0; i < numObjects; i++)
    {
        objects[i].lightBaseIdx = numLightColors;
        numLightColors += getNumTiles(&objects[i]);
    }
    lightColors = (Vector3*) malloc( numLightColors * sizeof(Vector3));
    for (int i = 0; i < numLightColors; i++)
        lightColors[i] = createVector3(0,0,0);

    vector<Rectangle> windows;
    for ( int i = 0; i < numObjects; i++)
    {
        //Color3 col = (*it)->getColor( (*it)->getTileCenter(0) );
        Vector3 col = objects[i].color;
        if (col[0] > 1 || col[1] > 1 || col[2] > 1)
            windows.push_back( objects[i]);
    }
    cout << "Registered " << windows.size() << " windows" << endl;

    
    for ( unsigned int i = 0; i < windows.size(); i++)
    {
        Rectangle window = windows[i];
        for (int j = 0; j < getNumTiles(&window); j++)
            lightColors[ window.lightBaseIdx + j] = createVector3(1, 1, 1);
            //getTile(&window, j).setLightColor(  );

        Vector3 src = getOrigin(       &window);
        Vector3 xDir= getWidthVector(  &window);
        Vector3 yDir= getHeightVector( &window);
        
        //float area = xDir.length() * yDir.length();
        float area = length(xDir) * length(yDir);
        cout << "Photon-Mapping window " << (i+1) << "/" << windows.size() << " with size " << area << endl;
         
        Vector3 xNorm = normalized(xDir);
        Vector3 yNorm = normalized(yDir);
        
        int numSamplesPerArea = 10;
        
        for (int i = 0; i < numSamplesPerArea*area; i++)
        {
            float dx = rand() / (float)RAND_MAX;
            float dy = rand() / (float)RAND_MAX;

            //Vector3 pos = src + xDir*dx + yDir*dy;
            Vector3 pos = add( src, add(mul(xDir, dx), mul(yDir, dy)));
            Vector3 n   = window.n;

            Vector3 ray_dir = getUniformDistributedRandomRay(n, xNorm, yNorm);
            pos = add(pos, mul(ray_dir, 1E-10f)); //to prevent self-intersection on the light source geometry
            Vector3 lightCol = window.color;
            
            for (int depth = 0; depth < 8; depth++)
            {
                
                float dist;
                Rectangle *hit = getClosestObject(pos, ray_dir, objects, numObjects, dist);
                if (!hit) continue;
                
                Vector3 hitPos = add(pos, mul(ray_dir, dist));
                //Tile& tile = getTileAt(hit, hitPos);
                Vector3 tileCol = hit->color;
                if (tileCol[0] > 1 || tileCol[1] > 1 || tileCol[2] > 1)    //hit a light source
                    continue;
                
                //Normalize light transfer by tile area and number of rays per area of the light source
                //the constant 2.0 is an experimentally-determined factor
                lightColors[ hit->lightBaseIdx + getTileIdAt(hit, hitPos)] +=
                    lightCol* (1 / (TILE_SIZE*TILE_SIZE*numSamplesPerArea*2));
                //Color3 light = tile.getLightColor() + lightCol* (1 / (TILE_SIZE*TILE_SIZE*numSamplesPerArea*2));
                //tile.setLightColor(light);
                
                lightCol[0] *= 0.9 * hit->color[0];// = Vector3( lightCol * ;//tile.getColor();
                lightCol[1] *= 0.9 * hit->color[1];
                lightCol[2] *= 0.9 * hit->color[2];
                // prepare next (diffuse reflective) ray
                Vector3 u,v;
                createBase(hit->n, /*out*/u, /*out*/v);
                ray_dir = getCosineDistributedRandomRay(hit->n, u, v);
                pos = hitPos;
            }
        }
    }

    char num[50];
    for ( int i = 0; i < numObjects; i++)
    {
        snprintf(num, 49, "%d", i);
        string filename = string("tiles/tile_") + num + ".png";
        saveAs( &objects[i], filename.c_str(), lightColors);
    }
    

    //Vector3 light_pos(1,1,1);
    Vector3 cam_pos = createVector3(420,882,120);
    Vector3 cam_dir = normalized( sub( createVector3(320, 205, 120), cam_pos));
    Vector3 cam_up  = createVector3(0,0,1);

    Vector3 cam_right = normalized( cross(cam_up,cam_dir) );
    cam_up   = normalized( cross(cam_right, cam_dir) );
    //std::cout << "cam_right: " << cam_right << endl;
    //std::cout << "cam_up: " << cam_up << endl;
    //std::cout << "cam_dir: " << cam_dir << endl;
    static const int img_width = 800;
    static const int img_height= 600;

    uint8_t* pixel_buffer = new uint8_t[3*img_width*img_height];
    memset(pixel_buffer, 0, 3*img_width*img_height);
    
    for (int y = 0; y < img_height; y++) 
    {
        /*if (y % 30 == 0)
        {
            cout << (y*100 / img_height) << "%" << endl;
            write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
        }*/
        for (int x = 0; x < img_width; x++) {
            /*Vector3 ray_dir = normalized( cam_dir + 
                                          cam_right* (1.25*(x-(img_width/2))/(double)img_width) + 
                                          cam_up*    (1.25*(img_height/(double)img_width)*(y-(img_height/2))/(double)img_height) );*/
            
            Vector3 rd_x = mul( cam_right,(1.25*(x-(img_width/2))/(float)img_width));
            Vector3 rd_y = mul( cam_up,   (1.25*(img_height/(float)img_width)*(y-(img_height/2))/(float)img_height));
            Vector3 ray_dir = normalized( add(cam_dir, add(rd_x, rd_y)));
            
            Vector3 col = getColor(cam_pos, ray_dir, objects, numObjects);
            /*col.r = sqrt(col.r);
            col.g = sqrt(col.g);
            col.b = sqrt(col.b);*/
            /*col.r = log(1+col.r) / log(2);  // conversion from light intensity to perceived brightness
            col.g = log(1+col.g) / log(2);
            col.b = log(1+col.b) / log(2);*/
            
            col[0] = 1 - exp(-col[0]);
            col[1] = 1 - exp(-col[1]);
            col[2] = 1 - exp(-col[2]);

          
          
            pixel_buffer[(y*img_width+x)*3 + 0] = min(col[0]*255, 255.0f);
            pixel_buffer[(y*img_width+x)*3 + 1] = min(col[1]*255, 255.0f);
            pixel_buffer[(y*img_width+x)*3 + 2] = min(col[2]*255, 255.0f);
        }
    }
    cout << "writing file with dimensions " << img_width << "x" << img_height << endl;
    write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
    
    delete [] pixel_buffer;
    //for ( int i = 0; i < numObjects; i++)
    //    delete objects[i].tiles;
    free( objects);
    free (lightColors);

    return 0;
}
