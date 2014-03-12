
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#include <list>
#include <GL/gl.h>

#include "vector3.h"
#include <stdlib.h> // for rand()

#include <math.h>
#include "sceneObject.h"

static const double TILE_SIZE = 10;

using namespace std;

class Rectangle : public SceneObject {

public:
    Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height):  pos(_pos), width(_width), height(_height), n(normalized(height.cross(width))) {
        r = 0.5;
        g = rand() / (double)RAND_MAX;
        b = rand() / (double)RAND_MAX;
    }

    double intersects( Vector3 ray_src, Vector3 ray_dir) {
        if (ray_dir.dot(n) > 0) return -1; //backface culling
        double denom = n.dot(ray_dir);
        if (denom == 0)
            return -1;
            
        double fac = n.dot( pos - ray_src ) / denom;
        if (fac < 0) return fac;    //is behind camera, cannot hit
        
        Vector3 p = ray_src + ray_dir * fac;
        Vector3 pDir = p - pos;
        
        double dx, dy;
        
        dx = normalized(width).dot(pDir);
        dy = normalized(height).dot(pDir);
        
        if (dx < 0 || dy < 0) return -1;
        if (dx*dx > width.squaredLength() || dy*dy > height.squaredLength()) return -1;
        return fac;
        
        
    }

    Vector3 normalAt(const Vector3&) {
        return n;
    }
    void render3d() const {
        int nHoriz = round(width.length() / TILE_SIZE);
        if (nHoriz < 1) nHoriz = 1;
        double tileFracH = 1/ (double)nHoriz;
        
        int nVert  = round(height.length() / TILE_SIZE);
        if (nVert < 1) nVert = 1;
        double tileFracV = 1/ (double)nVert;
        //double tile
        
        //cout << "split into " << nHoriz << "x" << nVert << " tiles" << endl;
        
        for (int x = 0; x < nHoriz; x++)
            for (int y = 0; y < nVert; y++) {
                glBegin(GL_TRIANGLES);
                    glColor3f(r*x*tileFracH, g*(y*tileFracV) , b);
                    //cout << "(" << x << ", " << y << ") " << x*tileFracH << " " << y*tileFracV << endl;

                    // d - c
                    // | / |
                    // a - b
                    Vector3 a = pos+ width*(x/(double)nHoriz) + height * (y/(double)nVert);
                    Vector3 b = a+   width*(1/(double)nHoriz);
                    Vector3 c = a+   width*(1/(double)nHoriz) + height * (1/(double)nVert);
                    Vector3 d = a                             + height * (1/(double)nVert);
                    
                    glVertex3f(a.x, a.y, a.z);
                    glVertex3f(b.x, b.y, b.z);
                    glVertex3f(c.x, c.y, c.z);

                    glVertex3f(a.x, a.y, a.z);
                    glVertex3f(c.x, c.y, c.z);
                    glVertex3f(d.x, d.y, d.z);
                glEnd();
            }
    }
    
    
    Color3 getColor() const { return Color3(r,g,b); };
    
private:
    Vector3 pos, width, height, n;
    double r, g, b;
};

list<Rectangle> parseLayout(const char* const filename);


#endif

