#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include "vector3.h"
#include "color3.h"
#include <math.h>
#include "assert.h"

static const double TILE_SIZE = 5;


class Tile {
public:
    Tile(const Color3& _col);
    Tile();
    
    Color3 getCombinedColor() const;

    void   setLightColor(const Color3& color);
    Color3 getLightColor() const;

    void   setColor(const Color3& color);
    Color3 getColor() const;
private:
    Color3 col;
    Color3 lightColor;
};

class SceneObject {
public:
    virtual ~SceneObject() {};
    virtual double intersects( Vector3 ray_src, Vector3 ray_dir, double closestDist = INFINITY) = 0; //return the distance of the closest intersection, or a negative value if no intersection exists
    virtual Vector3 getNormalAt(const Vector3 &pos) const = 0;
    virtual Color3 getColor(const Vector3 &pos) const = 0;
    virtual int getNumTiles() const = 0;
    virtual int getTileIdAt(const Vector3 &pos) const = 0;
    virtual Vector3 getTileCenter(int id) const = 0;
    virtual void setTileColor(const int tile_id, const Color3 &color) = 0;
};

class Plane: public SceneObject {

public:
    Plane( const Vector3 &_pos, const Vector3 &_n, const Color3 &_col = Color3(0.5, 0.5, 0.5) );
    double intersects( Vector3 ray_src, Vector3 ray_dir, double closestDist = INFINITY);
    Vector3 getNormalAt(const Vector3&) const       ;
    int getNumTiles() const                 ;
    int getTileIdAt(const Vector3 &) const  ;
    Vector3 getTileCenter(int) const        ;
    Color3 getColor(const Vector3 &) const ;
    void setTileColor(const int, const Color3 &color) ;
private:
    Vector3 pos, n;
    Tile    tile;
};

class Rectangle : public SceneObject {

public:
    Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height);
    Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height, const Color3 &col);
    double intersects( Vector3 ray_src, Vector3 ray_dir, double closestDist = INFINITY);
    Vector3 getNormalAt(const Vector3&) const;    
    int getNumTiles() const;    
    int getTileIdAt(const Vector3 &p) const;    
    Vector3 getTileCenter(int id) const;
    Color3 getColor(const Vector3 &pos) const;    
    void setTileColor(const int tileId, const Color3 &color);
    Tile&   getTile(const Vector3 &pos);
    Tile&   getTile(const int tileId);
    Vector3 getOrigin() const { return pos; }
    Vector3 getWidthVector() const { return width; }
    Vector3 getHeightVector() const { return height;}
    void saveAs(const char *filename) const;
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
