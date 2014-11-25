#ifndef VECTOR3_CL_H
#define VECTOR3_CL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
    #include <OpenCL/cl.h>
#else
    #include <CL/cl_platform.h>
#endif

typedef cl_float4 Vector3;

Vector3 initVector3(float x, float y, float z);
Vector3 add(const Vector3 a, const Vector3 b);
Vector3 add3(const Vector3 a, const Vector3 b, const Vector3 c);
Vector3 add4(const Vector3 a, const Vector3 b, const Vector3 c, const Vector3 d);
Vector3 sub(const Vector3 a, const Vector3 b);
Vector3 mul(const Vector3 b, float a);
Vector3 neg(const Vector3 v);
Vector3 div_vec3(const Vector3 a, float b);

Vector3 createVector3(float x, float y, float z);
Vector3 vec3(float x, float y, float z);
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
