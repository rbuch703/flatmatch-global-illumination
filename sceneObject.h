#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include "vector3.h"
#include "color3.h"
#include <math.h>

class Tile {
public:
    Tile(const Color3& _col): col(_col) {}
    Tile(): col(0.5, rand() / (double)RAND_MAX, rand() / (double)RAND_MAX) {};
    
    Color3 getColor() const{ return col;}
private:
    Color3 col;
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

    
    Color3 getColor(const Vector3 &) const { return tile.getColor(); };
    
private:
    Vector3 pos, n;
    //Color3       col;
    Tile    tile;
};
/*
class Sphere: public SceneObject {
public:
    Sphere( const Vector3 &_pos, double _r, const Color3 &_col = Color3(0.5, 0.5, 0.5) ):  pos(_pos), r(_r), col(_col) {}

    double intersects( Vector3 ray_src, Vector3 ray_dir) {
        Vector3 center_line = ray_src - pos;
        double tmp = ray_dir.dot(center_line);
        double s1 = tmp * tmp;
        double s2 = center_line.squaredLength();
        double q = s1 - s2 + r * r;

        if (q < 0)
            return -1.0;
            
        double d1 = - tmp + sqrt(q);
        double d2 = - tmp - sqrt(q);
        
        if (d1 < 0) 
            return d2;

        if (d2 < 0) 
            return d1;
            
        return d1 < d2 ? d1 : d2;
    }

    Vector3 normalAt(const Vector3 &p) {
        return (p - pos).normalized();
    }

    virtual Color3 getColor() const { return col; };
    
private:
    Vector3     pos;
    double      r;
    Color3      col;
};*/


#endif
