
#include "vector3_cl.h"

#include <math.h>

//typedef cl_float3 Vector3;


Vector3 add(Vector3 a, Vector3 b) { Vector3 res = { .s = {a.s[0]+b.s[0], a.s[1]+b.s[1], a.s[2]+b.s[2]} }; return res;}
Vector3 sub(Vector3 a, Vector3 b) { Vector3 res = { .s = {a.s[0]-b.s[0], a.s[1]-b.s[1], a.s[2]-b.s[2]} }; return res;}

Vector3 add3(const Vector3 a, const Vector3 b, const Vector3 c)
{
    Vector3 res = { .s = {a.s[0] + b.s[0] + c.s[0], 
                          a.s[1] + b.s[1] + c.s[1], 
                          a.s[2] + b.s[2] + c.s[2]} };   
    return res;
}

Vector3 add4(const Vector3 a, const Vector3 b, const Vector3 c, const Vector3 d)
{
    Vector3 res = { .s = {a.s[0] + b.s[0] + c.s[0] + d.s[0], 
                          a.s[1] + b.s[1] + c.s[1] + d.s[1], 
                          a.s[2] + b.s[2] + c.s[2] + d.s[2]} };   
    return res;
}


Vector3 initVector3(float x, float y, float z)
{
    Vector3 res = {.s = {x, y, z}};
    return res;
}


Vector3 mul(const Vector3 b, float a) { 
    Vector3 res = {.s = {b.s[0]*a, 
                         b.s[1]*a, 
                         b.s[2]*a} }; 
    return res;
}

Vector3 div_vec3(const Vector3 a, float b) { 
    float rec = 1.0f/b;
    Vector3 res = {.s = {a.s[0]*rec, 
                         a.s[1]*rec,
                         a.s[2]*rec} }; 
    return res;
}

Vector3 neg(const Vector3 v)
{
    Vector3 res = {.s = {-v.s[0], -v.s[1], -v.s[2]} };
    return res;
}


/*inline Vector3 diva(Vector3 a, float b) { 
    __m128 tmp = _mm_load1_ps(&b);
    return _mm_div_ps(a, tmp);
}*/

Vector3 createVector3(float _x, float _y, float _z)
{
    //__attribute__ ((aligned (16))) float tmp[4] = {x,y,z,0.0};
    //return _mm_load_ps(tmp);
    Vector3 res = {.s = {_x, _y, _z} };
    return res;
}

float dot(const Vector3 a, const Vector3 b)
{ 
    return a.s[0]*b.s[0] + a.s[1]*b.s[1] + a.s[2]*b.s[2];
}

Vector3 cross(const Vector3 a, const Vector3 b)      //copied from the internet, not verified
{
    Vector3 res = {.s = { a.s[1]*b.s[2] - a.s[2]*b.s[1], 
                          a.s[2]*b.s[0] - a.s[0]*b.s[2], 
                          a.s[0]*b.s[1] - a.s[1]*b.s[0]} };
  return res;
}

float squaredLength(const Vector3 a) 
{
    return a.s[0]*a.s[0] + a.s[1]*a.s[1] + a.s[2]*a.s[2];
}

float length(const Vector3 a) { return sqrtf( a.s[0]*a.s[0] + a.s[1]*a.s[1] + a.s[2]*a.s[2] ); }

Vector3 normalized(const Vector3 a) {
    float fac = 1.0f / length(a);
    Vector3 res = {.s = {a.s[0] * fac, 
                         a.s[1] * fac, 
                         a.s[2] * fac } };
    return res;
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

