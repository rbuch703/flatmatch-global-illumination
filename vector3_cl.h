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

void inc(Vector3 *a, const Vector3 b);  //increment

Vector3 createVector3(float x, float y, float z);
Vector3 vec3(float x, float y, float z);
float dot(const Vector3 a, const Vector3 b);
Vector3 cross(const Vector3 a, const Vector3 b);
float squaredLength(const Vector3 a);
float length(const Vector3 a);
Vector3 normalized(const Vector3 a);


//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void createBase(                      const Vector3 ndir, Vector3 *c1, Vector3 *c2);
Vector3 getCosineDistributedRandomRay(const Vector3 ndir);
Vector3 getDiffuseSkyRandomRay(       const Vector3 ndir);

#ifdef __cplusplus
}
#endif


#endif
