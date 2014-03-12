
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#include <list>
//#include <GL/gl.h>

#include "vector3.h"
#include <stdlib.h> // for rand()

#include <math.h>
#include "sceneObject.h"
#include "assert.h"

static const double TILE_SIZE = 10;

using namespace std;

class Rectangle : public SceneObject {

public:
    Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height):  
            pos(_pos), width(_width), height(_height), 
            n(normalized(height.cross(width))),
            width_norm (normalized(width)),
            height_norm (normalized(height)),
            hLength( width.length()),
            vLength(height.length()) {
        r = 0.5;
        g = rand() / (double)RAND_MAX;
        b = rand() / (double)RAND_MAX;
        
        hNumTiles = round(width.length() / TILE_SIZE);
        if (hNumTiles < 1) hNumTiles = 1;
        
        vNumTiles = round(height.length()/ TILE_SIZE);
        if (vNumTiles < 1) vNumTiles = 1;
        
        tiles = new Tile[vNumTiles * hNumTiles];
        
        width_per_tile = width / hNumTiles;
        height_per_tile= height/ vNumTiles;
        //numTiles = ;
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
        
        dx = width_norm.dot(pDir);
        dy = height_norm.dot(pDir);
        
        if (dx < 0 || dy < 0) return -1;
        if (dx > hLength || dy > vLength) return -1;
        return fac;
        
        
    }

    Vector3 normalAt(const Vector3&) {
        return n;
    }
    
    int getNumTiles() const
    {
        return hNumTiles * vNumTiles;
    }
    
    int getTileIdAt(const Vector3 &p) const
    {
        Vector3 pDir = p - pos; //vector from rectangle origin (its lower left corner) to current point
        double dx, dy;
        
        dx = width_norm.dot(pDir);
        dy = height_norm.dot(pDir);
        int tx = (dx*hNumTiles) / hLength;
        int ty = (dy*vNumTiles) / vLength;
        if (tx < 0) tx = 0;
        if (tx >= hNumTiles) tx = hNumTiles - 1;
        if (ty < 0) ty = 0;
        if (ty >= vNumTiles) ty = vNumTiles - 1;
        
        assert(ty*hNumTiles + tx <= getNumTiles());
        return ty*hNumTiles + tx;
    }
    
    Vector3 getTileCenter(int id) const
    {
        int ty = id / hNumTiles;
        int tx = id % hNumTiles;

        if (tx < 0) tx = 0;
        if (tx >= hNumTiles) tx = hNumTiles - 1;
        if (ty < 0) ty = 0;
        if (ty >= vNumTiles) ty = vNumTiles - 1;

        return pos + width_per_tile * (tx+0.5) + height_per_tile * (ty+0.5);
        //return pos;
    }
    
#if 0    
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
#endif    
    
    Color3 getColor(const Vector3 &pos) const 
    { 
        int tile_id = getTileIdAt(pos);
        Vector3 p = getTileCenter(tile_id);
        
        double dist = (pos - p).length();
        //cout << dist << endl;
        double fac = (1-dist/7.0);
        return tiles[tile_id].getColor()*fac*fac;
        //return Color3(r,g,b); 
    
    };
    
private:
    Vector3 pos, width, height, n;
    Vector3 width_norm, height_norm;
    Vector3 width_per_tile, height_per_tile;
    double r, g, b;
    double hLength, vLength;
    
    int hNumTiles, vNumTiles;
    Tile* tiles;
};

list<Rectangle> parseLayout(const char* const filename);


#endif

