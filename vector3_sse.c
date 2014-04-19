#ifndef VECTOR3_SSE_H
#define VECTOR3_SSE_H

#include "vector3_sse.h"

//#include <iostream>
#include <math.h>
//#include <pmmintrin.h>
#include <x86intrin.h>

typedef __m128 Vector3;

//#include <math.h>

/* the vector3 class is a performance hotspot. It should therefore stay in a 
header file in order to allow for more agressive inlining by the compiler*/

Vector3 add(const Vector3 a, const Vector3 b) { return _mm_add_ps(a, b); }
Vector3 sub(const Vector3 a, const Vector3 b) { return _mm_sub_ps(a, b); }

/*inline Vector3 mul(float a, const Vector3 b) { 
    __m128 tmp = _mm_load1_ps(&a);
    return _mm_mul_ps(tmp, b);
}*/

Vector3 mul(const Vector3 b, float a) { 
    __m128 tmp = _mm_load1_ps(&a);
    return _mm_mul_ps(tmp, b);
}

Vector3 div_vec3(const Vector3 a, float b) { 
    __m128 tmp = _mm_load1_ps(&b);
    return _mm_div_ps(a, tmp);
}


/*inline Vector3 diva(Vector3 a, float b) { 
    __m128 tmp = _mm_load1_ps(&b);
    return _mm_div_ps(a, tmp);
}*/

Vector3 createVector3(float x, float y, float z)
{
    __attribute__ ((aligned (16))) float tmp[4] = {x,y,z,0.0};
    return _mm_load_ps(tmp);
}

float dot(const Vector3 a, const Vector3 b)     //copied from the internet, not verified
{ 
    __m128 r1 = _mm_mul_ps(a, b);
    __m128 r2 = _mm_hadd_ps(r1, r1);
    __m128 r3 = _mm_hadd_ps(r2, r2);
    float result;
    _mm_store_ss(&result, r3);
    return result;
}

Vector3 cross(const Vector3 a, const Vector3 b)      //copied from the internet, not verified
{
  return _mm_sub_ps(
    _mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2))),
    _mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1)))
  );
}

float squaredLength(const Vector3 a) 
{
    return dot(a,a);    
}

float length(const Vector3 a) { return sqrtf( dot(a,a) ); }

Vector3 normalized(const Vector3 a) {
    float len = length(a);
    return _mm_div_ps( a, _mm_load1_ps(&len));
}


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


#endif
