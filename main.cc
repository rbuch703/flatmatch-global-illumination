
#include "vector3.h"
#include "sceneObject.h"
#include "parseLayout.h"

#include <list>
#include <iostream>
#include <stdint.h>
#include <stdlib.h> //for rand()
#include <string>
using namespace std;

void read_png_file(const char* file_name, int &width, int& height, int& color_type, uint8_t* &pixel_buffer );
void write_png_file(const char* file_name, int width, int height, int color_type, uint8_t *pixel_buffer);
static const int PNG_COLOR_TYPE_RGB  = 2;
static const int PNG_COLOR_TYPE_RGBA = 6;

list<SceneObject*> objects;


Vector3 getRandomRay(Vector3 ndir, Vector3 udir, Vector3 vdir) {
    //# Compute a uniformly distributed point on the unit disk
    double r = sqrt(rand() / (double)RAND_MAX);
    double phi = 2 * 3.141592 * rand() / (double)RAND_MAX;

    //# Project point onto unit hemisphere
    double u = r * cos(phi);
    double v = r * sin(phi);
    double n = sqrt(1 - r*r);

    //# Convert to a direction on the hemisphere defined by the normal
    return udir*u + vdir*v + ndir*n;
    
}

//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void getBase(Vector3 ndir, Vector3 &c1, Vector3 &c2) {
    c1 = Vector3(0,0,1);
    if (fabs(c1.dot(ndir)) == 1) //are colinear --> cannot build coordinate base
        c1 = Vector3(0,1,0);
        
    c2 = normalized( c1.cross(ndir));
    c1 = normalized( c2.cross(ndir));
}

SceneObject* getClosestObject(const Vector3 &ray_src, const Vector3 &ray_dir, const list<SceneObject*> &objects, double &dist_out)
{
    SceneObject *closestObject = NULL;
    dist_out = INFINITY;

    for ( list<SceneObject*>::const_iterator it = objects.begin(); it != objects.end(); it++)
    {
        double dist = (*it)->intersects( ray_src, ray_dir);
        if (dist < 0)
            continue;
            
        if (dist < dist_out) {
            closestObject = *it;
            dist_out = dist;
        }
    }
    return closestObject;
}

Color3 getColor(Vector3 ray_src, Vector3 ray_dir, const list<SceneObject*> &objects, int depth = 0)
{

    double hitDist;
    SceneObject *hitObject = getClosestObject(ray_src, ray_dir, objects, hitDist);

    if (! hitObject) //no hit --> simulate white area light source with direction (-1,-2,-1)
    {
        return Color3(0,0,0);
        //double c = normalized(-Vector3(-1, -2, -1)).dot(ray_dir);
        //return Color3(c,c,c);
    }

    Vector3 intersect_pos = ray_src + ray_dir * hitDist;

    Color3 color = hitObject->getColor(intersect_pos);
    //return color;
    if (color.r > 1 || color.g > 1 || color.b > 1) return color; //is an emitter    

    /*double diffuse = (-ray_dir).dot(closestObject->normalAt(intersect_pos));
    if (diffuse <0) diffuse = 0;
    return color * diffuse;*/
    //return Color3(diffuse, diffuse, diffuse);


    if ((depth >= 8) or (color == Color3(0,0,0)))
        return Color3(0,0,0);

    
    Vector3 udir(0,0,0);
    Vector3 vdir(0,0,0);
    Vector3 n = hitObject->normalAt(intersect_pos);
    getBase( n, /*ref*/udir, /*ref*/vdir);
    
    
    ray_dir = getRandomRay( n, udir, vdir );
    ray_src = intersect_pos + ray_dir * 1E-6;
    
    Color3 c2 = getColor( ray_src, ray_dir, objects, depth+1);

    return Color3( color.r * c2.r * 0.8,
                   color.g * c2.g * 0.8, 
                   color.b * c2.b * 0.8);
    
}

int main()
{
    objects.push_back( new Rectangle( Vector3(0,0,0), Vector3(0, 1000, 0), Vector3(1000, 0, 0), Color3(0.5, 0.5, 0.5)));    // floor
    objects.push_back( new Rectangle( Vector3(0,0,200), Vector3(1000, 0, 0), Vector3(0, 1000, 0), Color3(0.5, 0.5, 0.5)));  // ceiling
    
    list<Rectangle> rects = parseLayout("layout.png");
    for ( list<Rectangle>::const_iterator it = rects.begin(); it != rects.end(); it++)
        objects.push_back( new Rectangle(*it));

    Vector3 light_pos(1,1,1);
    Vector3 cam_pos(420,882,120);
    Vector3 cam_dir  = normalized(Vector3(320, 205, 120) - cam_pos) ;
    Vector3 cam_up(0,0,1);

    Vector3 cam_right = normalized( cam_up.cross(cam_dir) );
    cam_up   = normalized( cam_right.cross(cam_dir) );
    std::cout << "cam_right: " << cam_right << endl;
    std::cout << "cam_up: " << cam_up << endl;
    std::cout << "cam_dir: " << cam_dir << endl;
    static const int img_width = 400;
    static const int img_height= 300;

#if 0
    for (list<SceneObject*>::iterator it = objects.begin(); it != objects.end(); it++)
    {
        SceneObject &obj = **it;
        for (int tileId = 0; tileId < obj.getNumTiles(); tileId++)
        {
            Vector3 pos = obj.getTileCenter(tileId);
            
            Color3 objCol = obj.getColor(pos);
            if (objCol.r > 1 || objCol.g > 1 || objCol.b > 1)
                continue; // is a light emitter itself, no need co compute occlusion
            
            double dist;
            SceneObject *hitObj = getClosestObject( pos, normalized(Vector3(0, -2, 1)), objects, dist);
            if (!hitObj) { /*obj.setTileColor(tileId, Color3(0,0,0));*/ continue;}

            Vector3 hitPos = pos + normalized(Vector3(0, -2, 1)) * dist;
            Color3 hitColor = hitObj->getColor( hitPos );
            if (hitColor.r < 1 && hitColor.g < 1 && hitColor.b < 1)
                continue;
                //obj.setTileColor(tileId, Color3(0,0,0));
            obj.setTileColor(tileId, Color3(2,2,2));
        }
    }
#endif
    uint8_t* pixel_buffer = new uint8_t[3*img_width*img_height];
    for (int y = 0; y < img_height; y++) 
    {
        if (y % 3 == 0)
        {
            cout << (y*100 / img_height) << "%" << endl;
            write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
        }
        for (int x = 0; x < img_width; x++) {
            Vector3 ray_dir = normalized( cam_dir + 
                                          cam_right* (1.25*(x-(img_width/2))/(double)img_width) + 
                                          cam_up*    (1.25*(img_height/(double)img_width)*(y-(img_height/2))/(double)img_height) );
            
            static const int NUM_ITERATIONS = 1000;
            Color3 col_acc(0,0,0);
            for (int i = 0; i < NUM_ITERATIONS; i++)
            {
                col_acc = col_acc + getColor(cam_pos, ray_dir, objects);
                
            }
            col_acc = col_acc / (double)NUM_ITERATIONS;

          
            pixel_buffer[(y*img_width+x)*3 + 0] = min(col_acc.r*255, 255.0);
            pixel_buffer[(y*img_width+x)*3 + 1] = min(col_acc.g*255, 255.0);
            pixel_buffer[(y*img_width+x)*3 + 2] = min(col_acc.b*255, 255.0);
        }
    }

    write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
    
    delete [] pixel_buffer;
    for ( list<SceneObject*>::iterator it = objects.begin(); it != objects.end(); it++)
        delete *it;
    return 0;
}
