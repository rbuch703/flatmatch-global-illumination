#ifndef VECTOR3_H
#define VECTOR3_H

#include <iostream>

class Vector3 {
public:
    Vector3( double _x, double _y, double _z);
    
    Vector3 operator+(const Vector3 &other) const;
    Vector3 operator-(const Vector3 &other) const;
    Vector3& operator+=(const Vector3 &other);
    Vector3& operator-=(const Vector3 &other);

    Vector3 operator-() const;

    Vector3 operator*(const double a) const;
    Vector3 operator/(const double a) const;

    double dot(const Vector3 &other)  const;
    Vector3 cross(const Vector3 &other) const;

    double squaredLength() const;
    double length() const;
    
    Vector3 normalized() const;
    Vector3 rotate_z(double angle) const;
    
private:
    friend std::ostream& operator<<(std::ostream &os, Vector3 v);
public:
    double x, y, z;
};

Vector3 normalized(const Vector3 &v);

#endif
