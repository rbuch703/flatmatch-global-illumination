#ifndef VECTOR3_SSE_H
#define VECTOR3_SSE_H

#ifdef __cplusplus
extern "C" {
#endif

//#include <iostream>
//#include <pmmintrin.h>

//#include <math.h>

/* the vector3 class is a performance hotspot. It should therefore stay in a 
header file in order to allow for more agressive inlining by the compiler*/
#include <x86intrin.h>

typedef __m128 Vector3;

Vector3 add(const Vector3 a, const Vector3 b);
Vector3 sub(const Vector3 a, const Vector3 b);
Vector3 mul(const Vector3 b, float a);
Vector3 div_vec3(const Vector3 a, float b);

Vector3 createVector3(float x, float y, float z);
float dot(const Vector3 a, const Vector3 b);
Vector3 cross(const Vector3 a, const Vector3 b);
float squaredLength(const Vector3 a);
float length(const Vector3 a);
Vector3 normalized(const Vector3 a);


/*Vector3 rotate_z(double angle) const
{
    double new_x = cos(angle)* x - sin(angle) * y;
    double new_y = sin(angle)* x + cos(angle) * y;
    return Vector3(new_x, new_y, z).normalized();
}*/

//    Vector3 cross(const Vector3 &other) const { return Vector3(y*other.z - z*other.y, z*other.x - x*other.z, x*other.y - y*other.x); }

/*inline std::ostream& operator<<(std::ostream &os, Vector3 v)
{
    __attribute__ ((aligned (16))) float tmp[4];
    _mm_store_ps(tmp, v);
    os << "(" << tmp[0] << ", " << tmp[1] << ", " << tmp[2] << ")";
    return os;
}*/

#ifdef __cplusplus
}
#endif


#endif