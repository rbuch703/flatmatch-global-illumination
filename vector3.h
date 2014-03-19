#ifndef VECTOR3_H
#define VECTOR3_H

#include <iostream>
#include <math.h>

/* the vector3 class is a performance hotspot. It should therefore stay in a 
header file in order to allow for more agressive inlining by the compiler*/

class Vector3 {
public:
    Vector3( ) {}
    //Vector3( const Vector3 &other ): x(other.x), y(other.y), z(other.z) {}  //copy constructor (faster without it)
    
    Vector3( double _x, double _y, double _z): x(_x), y(_y), z(_z) {}
        
    Vector3  operator+(const Vector3 &other) const { return Vector3( x+other.x, y+other.y, z+other.z); }
    Vector3  operator-(const Vector3 &other) const { return Vector3( x-other.x, y-other.y, z-other.z); }
    Vector3& operator+=(const Vector3 &other)
    { 
        x += other.x;
        y += other.y;
        z += other.z;
        return *this; 
    }

    Vector3& operator-=(const Vector3 &other)
    { 
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this; 
    }


    Vector3 operator-() const { return Vector3(-x, -y, -z); }

    Vector3 operator*(const double a) const { return Vector3( x*a, y*a, z*a); }
    Vector3 operator/(const double a) const { return Vector3( x/a, y/a, z/a); }

    double dot(const Vector3 &other)  const { return x*other.x + y*other.y + z*other.z;}
    Vector3 cross(const Vector3 &other) const { return Vector3(y*other.z - z*other.y, z*other.x - x*other.z, x*other.y - y*other.x); }

    double squaredLength() const { return x*x+y*y+z*z; }

    double length() const { return sqrt(x*x+y*y+z*z); }
        
    Vector3 normalized() const { 
        double len = sqrt(x*x+y*y+z*z);
        return *this / len;
    }
    
    Vector3 rotate_z(double angle) const
    {
        double new_x = cos(angle)* x - sin(angle) * y;
        double new_y = sin(angle)* x + cos(angle) * y;
        return Vector3(new_x, new_y, z).normalized();
    }
    
private:
    friend std::ostream& operator<<(std::ostream &os, Vector3 v);
public:
    double x, y, z;
};

Vector3 normalized(const Vector3 &v);


/*
static std::ostream& operator<<(std::ostream &os, Vector3 v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}*/


#endif
