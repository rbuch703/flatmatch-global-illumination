#ifndef VECTOR3_H
#define VECTOR3_H

#include <math.h>
#include <iostream>

using namespace std;

class Vector3 {
public:
    Vector3( double _x, double _y, double _z): x(_x), y(_y), z(_z) {}
    
    Vector3 operator+(const Vector3 &other) const { return Vector3( x+other.x, y+other.y, z+other.z); }
    Vector3 operator-(const Vector3 &other) const { return Vector3( x-other.x, y-other.y, z-other.z); }

    Vector3 operator-() const { return Vector3(-x, -y, -z); }

    Vector3 operator*(const double a) const { return Vector3( x*a, y*a, z*a); }
    Vector3 operator/(const double a) const { return Vector3( x/a, y/a, z/a); }

    double dot(const Vector3 &other)  const { return x*other.x + y*other.y + z*other.z;}
    Vector3 cross(const Vector3 &other) const { return Vector3(y*other.z - z*other.y, z*other.x - x*other.z, x*other.y - y*other.x); }

    double squaredLength() const { return x*x+y*y+z*z; }
    
    Vector3 normalized() const { 
        double len = sqrt(this->dot(*this));
        return *this / len;
    }
private:
    friend std::ostream& operator<<(std::ostream &os, Vector3 v);
    double x, y, z;
};

Vector3 normalized(const Vector3 &v) { return v.normalized();}


std::ostream& operator<<(std::ostream &os, Vector3 v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}


#endif
