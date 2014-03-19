
#include "vector3.h"
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


vector<Rectangle*> objects;



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

Rectangle* getClosestObject(const Vector3 &ray_src, const Vector3 &ray_dir, const vector<Rectangle*> &objects, double &dist_out)
{
    Rectangle *closestObject = NULL;
    dist_out = INFINITY;
    
    int numObjects = objects.size();
    
    for ( int i = 0; i < numObjects; i++)
    {
        double dist = objects[i]->intersects( ray_src, ray_dir, dist_out);
        if (dist < 0)
            continue;
            
        if (dist < dist_out) {
            closestObject = objects[i];
            dist_out = dist;
        }
    }
    return closestObject;
}

Color3 getColor(Vector3 ray_src, Vector3 ray_dir, const vector<Rectangle*> &objects/*, int depth = 0*/)
{

    double hitDist;
    Rectangle *hitObject = getClosestObject(ray_src, ray_dir, objects, hitDist);

    if (! hitObject)         
        return Color3(0,0,0);

    Vector3 intersect_pos = ray_src + ray_dir * hitDist;
    return hitObject->getColor(intersect_pos);
}

int main()
{
    static const Color3 wallColor(0.8, 0.8, 0.8);
    objects.push_back( new Rectangle( Vector3(0,0,0), Vector3(0, 1000, 0), Vector3(1000, 0, 0), wallColor));    // floor
    objects.push_back( new Rectangle( Vector3(0,0,200), Vector3(1000, 0, 0), Vector3(0, 1000, 0), wallColor));  // ceiling
    
    vector<Rectangle> rects = parseLayout("layout.png");
    for ( vector<Rectangle>::const_iterator it = rects.begin(); it != rects.end(); it++)
        objects.push_back( new Rectangle(*it));

    vector<Rectangle*> windows;
    for ( vector<Rectangle*>::const_iterator it = objects.begin(); it != objects.end(); it++)
    {
        //Color3 col = (*it)->getColor( (*it)->getTileCenter(0) );
        Color3 col = ((Rectangle*)(*it))->getTile((*it)->getTileCenter(0)).getColor();
        if (col.r > 1 || col.g > 1 || col.b > 1)
            windows.push_back( *it);
    }
    cout << "Registered " << windows.size() << " windows" << endl;

    
    for ( unsigned int i = 0; i < windows.size(); i++)
    {
        Rectangle &window = *(windows[i]);
        for (int j = 0; j < window.getNumTiles(); j++)
            window.getTile(j).setLightColor( Color3(1, 1, 1) );

        Vector3 src = window.getOrigin();
        Vector3 xDir= window.getWidthVector();
        Vector3 yDir= window.getHeightVector();
        
        double area = xDir.length() * yDir.length();
        cout << "Photon-Mapping window " << (i+1) << "/" << windows.size() << " with size " << area << endl;
         
        Vector3 xNorm = normalized(xDir);
        Vector3 yNorm = normalized(yDir);
        
        int numSamplesPerArea = 1000;
        
        for (int i = 0; i < numSamplesPerArea*area; i++)
        {
            double dx = rand() / (double)RAND_MAX;
            double dy = rand() / (double)RAND_MAX;

            Vector3 pos = src + xDir*dx + yDir*dy;
            Vector3 n   = window.getNormalAt( pos ); 

            Vector3 ray_dir = getRandomRay(n, xNorm, yNorm);
            pos = pos + ray_dir * 1E-10; //to prevent self-intersection on the light source geometry
            Color3 lightCol = window.getColor( pos );
            
            for (int depth = 0; depth < 6; depth++)
            {
                
                double dist;
                Rectangle *hit = getClosestObject(pos, ray_dir, objects, dist);
                if (!hit) continue;
                
                Vector3 hitPos = pos + ray_dir * dist;
                Tile& tile = ((Rectangle*)hit)->getTile(hitPos);
                Color3 tileCol = tile.getColor();
                if (tileCol.r > 1 || tileCol.g > 1 || tileCol.b > 1)    //hit a light source
                    continue;
                
                //Normalize light transfer by tile area and number of rays per area of the light source
                //the constant 2.0 is an experimentally-determined factor
                Color3 light = tile.getLightColor() + lightCol* (1 / (TILE_SIZE*TILE_SIZE*numSamplesPerArea*2));
                tile.setLightColor(light);
                
                lightCol = lightCol * 0.9 * tile.getColor();
                
                // prepare next (diffuse reflective) ray
                n = hit->getNormalAt(hitPos);
                Vector3 u,v;
                getBase(n, /*out*/u, /*out*/v);
                ray_dir = getRandomRay(n, u, v);
                pos = hitPos;
            }
        }
    }

    int idx = 0;
    char num[50];
    for ( vector<Rectangle*>::const_iterator it = objects.begin(); it != objects.end(); it++)
    {
        snprintf(num, 49, "%d", idx);
        string filename = string("tiles/tile_") + num + ".png";
        (*it)->saveAs(filename.c_str());
        idx++;
    }
    

    //Vector3 light_pos(1,1,1);
    Vector3 cam_pos(420,882,120);
    Vector3 cam_dir  = normalized(Vector3(320, 205, 120) - cam_pos) ;
    Vector3 cam_up(0,0,1);

    Vector3 cam_right = normalized( cam_up.cross(cam_dir) );
    cam_up   = normalized( cam_right.cross(cam_dir) );
    //std::cout << "cam_right: " << cam_right << endl;
    //std::cout << "cam_up: " << cam_up << endl;
    //std::cout << "cam_dir: " << cam_dir << endl;
    static const int img_width = 800;
    static const int img_height= 600;

    uint8_t* pixel_buffer = new uint8_t[3*img_width*img_height];
    memset(pixel_buffer, 0, 3*img_width*img_height);
    
    for (int y = 0; y < img_height; y++) 
    {
        if (y % 30 == 0)
        {
            cout << (y*100 / img_height) << "%" << endl;
            write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
        }
        for (int x = 0; x < img_width; x++) {
            Vector3 ray_dir = normalized( cam_dir + 
                                          cam_right* (1.25*(x-(img_width/2))/(double)img_width) + 
                                          cam_up*    (1.25*(img_height/(double)img_width)*(y-(img_height/2))/(double)img_height) );
            
            Color3 col = getColor(cam_pos, ray_dir, objects);
            /*col.r = sqrt(col.r);
            col.g = sqrt(col.g);
            col.b = sqrt(col.b);*/
            col.r = log(1+col.r) / log(2);  // conversion from light intensity to perceived brightness
            col.g = log(1+col.g) / log(2);
            col.b = log(1+col.b) / log(2);

          
          
            pixel_buffer[(y*img_width+x)*3 + 0] = min(col.r*255, 255.0);
            pixel_buffer[(y*img_width+x)*3 + 1] = min(col.g*255, 255.0);
            pixel_buffer[(y*img_width+x)*3 + 2] = min(col.b*255, 255.0);
        }
    }

    write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
    
    delete [] pixel_buffer;
    for ( vector<Rectangle*>::iterator it = objects.begin(); it != objects.end(); it++)
        delete *it;
    return 0;
}
