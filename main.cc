
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


list<Rectangle*> objects;



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

Rectangle* getClosestObject(const Vector3 &ray_src, const Vector3 &ray_dir, const list<Rectangle*> &objects, double &dist_out)
{
    Rectangle *closestObject = NULL;
    dist_out = INFINITY;

    for ( list<Rectangle*>::const_iterator it = objects.begin(); it != objects.end(); it++)
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

Color3 getColor(Vector3 ray_src, Vector3 ray_dir, const list<Rectangle*> &objects, int depth = 0)
{

    double hitDist;
    Rectangle *hitObject = getClosestObject(ray_src, ray_dir, objects, hitDist);

    if (! hitObject) //no hit --> simulate white area light source with direction (-1,-2,-1)
    {
        return Color3(0,0,0);
        //double c = normalized(-Vector3(-1, -2, -1)).dot(ray_dir);
        //return Color3(c,c,c);
    }

    Vector3 intersect_pos = ray_src + ray_dir * hitDist;

    Color3 color = hitObject->getColor(intersect_pos);
    return color;
    if (color.r > 1 || color.g > 1 || color.b > 1) return color; //is an emitter    

    /*double diffuse = (-ray_dir).dot(closestObject->getNormalAt(intersect_pos));
    if (diffuse <0) diffuse = 0;
    return color * diffuse;*/
    //return Color3(diffuse, diffuse, diffuse);


    if ((depth >= 4) or (color == Color3(0,0,0)))
        return Color3(0,0,0);

    
    Vector3 udir(0,0,0);
    Vector3 vdir(0,0,0);
    Vector3 n = hitObject->getNormalAt(intersect_pos);
    getBase( n, /*ref*/udir, /*ref*/vdir);
    
    
    ray_dir = getRandomRay( n, udir, vdir );
    ray_src = intersect_pos + ray_dir * 1E-6;
    
    Color3 c2 = getColor( ray_src, ray_dir, objects, depth+1);

    return color*c2*0.7;    
}

int main()
{
    static const Color3 wallColor(0.8, 0.8, 0.8);
    objects.push_back( new Rectangle( Vector3(0,0,0), Vector3(0, 1000, 0), Vector3(1000, 0, 0), wallColor));    // floor
    objects.push_back( new Rectangle( Vector3(0,0,200), Vector3(1000, 0, 0), Vector3(0, 1000, 0), wallColor));  // ceiling
    
    list<Rectangle> rects = parseLayout("layout.png");
    for ( list<Rectangle>::const_iterator it = rects.begin(); it != rects.end(); it++)
        objects.push_back( new Rectangle(*it));

    vector<Rectangle*> windows;
    for ( list<Rectangle*>::const_iterator it = objects.begin(); it != objects.end(); it++)
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
        
        // is guaranteed to have the same normal everywhere, so just take that one of the first tile
        
        Vector3 xNorm = normalized(xDir);
        Vector3 yNorm = normalized(yDir);
        assert( n.dot(xNorm) < 1E-10);      //all need to be perpendicular to form an orthogonal base
        assert( n.dot(yNorm) < 1E-10);
        assert( xNorm.dot(yNorm) < 1E-10);
        
        
        int numSamplesPerArea = 1000*50;
        
        for (int i = 0; i < numSamplesPerArea*area; i++)
        {
            double dx = rand() / (double)RAND_MAX;
            double dy = rand() / (double)RAND_MAX;
            //Color3 col = window.getColor(pos);    //TODO: use light emitter color for lighting instead of fixed light brightness
            Vector3 pos = src + xDir*dx + yDir*dy;
            Vector3 n   = window.getNormalAt( pos ); 

            Vector3 ray_dir = getRandomRay(n, xNorm, yNorm);
            Color3 lightCol = window.getColor( pos );
            
            for (int depth = 0; depth < 4; depth++)
            {
                
                double dist;
                Rectangle *hit = getClosestObject(pos, ray_dir, objects, dist);
                if (!hit) continue;
                
                Vector3 hitPos = pos + ray_dir * dist;
                Tile& tile = ((Rectangle*)hit)->getTile(hitPos);
                Color3 light = tile.getLightColor() + lightCol* (4/(TILE_SIZE*TILE_SIZE*numSamplesPerArea*12.5));
                tile.setLightColor(light);
                
                lightCol = lightCol * 0.7 * tile.getColor();
                
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
    for ( list<Rectangle*>::const_iterator it = objects.begin(); it != objects.end(); it++)
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
    std::cout << "cam_right: " << cam_right << endl;
    std::cout << "cam_up: " << cam_up << endl;
    std::cout << "cam_dir: " << cam_dir << endl;
    static const int img_width = 1920;
    static const int img_height= 1080;

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
            
            static const int NUM_ITERATIONS = 1;
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
    for ( list<Rectangle*>::iterator it = objects.begin(); it != objects.end(); it++)
        delete *it;
    return 0;
}
