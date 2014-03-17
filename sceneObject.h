#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include "vector3.h"
#include "color3.h"
#include <math.h>
#include "assert.h"

class Tile {
public:
    Tile(const Color3& _col): col(_col), lightColor(1,1,1) {}
    Tile(): col(0.5, rand() / (double)RAND_MAX, rand() / (double)RAND_MAX), lightColor(1,1,1) {};
    
    Color3 getCombinedColor() const{ return col*lightColor;}

    void   setLightColor(const Color3& color) { lightColor = color;}
    Color3 getLightColor() const { return lightColor;}

    void   setColor(const Color3& color) { col = color;}
    Color3 getColor() const { return col;}

private:
    Color3 col;
    Color3 lightColor;
};

class SceneObject {
public:
    virtual ~SceneObject() {};
    virtual double intersects( Vector3 ray_src, Vector3 ray_dir) = 0; //return the distance of the closest intersection, or a negative value if no intersection exists
    virtual Vector3 normalAt(const Vector3 &pos) = 0;
    virtual Color3 getColor(const Vector3 &pos) const = 0;
    virtual int getNumTiles() const = 0;
    virtual int getTileIdAt(const Vector3 &pos) const = 0;
    virtual Vector3 getTileCenter(int id) const = 0;
    virtual void setTileColor(const int tile_id, const Color3 &color) = 0;
};

class Plane: public SceneObject {

public:
    Plane( const Vector3 &_pos, const Vector3 &_n, const Color3 &_col = Color3(0.5, 0.5, 0.5) ):  pos(_pos), n(_n), tile(_col) {}

    double intersects( Vector3 ray_src, Vector3 ray_dir) {
        double denom = n.dot(ray_dir);
        if (denom == 0)
            return -1;
            
        return n.dot( pos - ray_src ) / denom;
    }

    Vector3 normalAt(const Vector3&)        { return n;   }

    int getNumTiles() const                 { return 1;   }
    int getTileIdAt(const Vector3 &) const  { return 0;   }
    Vector3 getTileCenter(int) const        { return pos; }

    
    Color3 getColor(const Vector3 &) const { return tile.getCombinedColor(); };

    void setTileColor(const int, const Color3 &color) {
        tile.setLightColor(color);
    }
    
private:
    Vector3 pos, n;
    Tile    tile;
};

class Rectangle : public SceneObject {

public:
    Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height);
    Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height, const Color3 &col);
    double intersects( Vector3 ray_src, Vector3 ray_dir);
    Vector3 normalAt(const Vector3&);    
    int getNumTiles() const;    
    int getTileIdAt(const Vector3 &p) const;    
    Vector3 getTileCenter(int id) const;
    Color3 getColor(const Vector3 &pos) const;    
    void setTileColor(const int tileId, const Color3 &color);
    
private:
    Vector3 pos, width, height, n;
    Vector3 width_norm, height_norm;
    Vector3 width_per_tile, height_per_tile;
    //double r, g, b;
    double hLength, vLength;
    
    int hNumTiles, vNumTiles;
    Tile* tiles;
};


#endif
