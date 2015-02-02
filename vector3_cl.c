
#include <math.h>
#include <assert.h>

#include "vector3_cl.h"


Vector3 add(Vector3 a, Vector3 b) 
{ 
    return (Vector3){ .s = {a.s[0]+b.s[0], a.s[1]+b.s[1], a.s[2]+b.s[2]} }; 
}

Vector3 sub(Vector3 a, Vector3 b) 
{ 
    return (Vector3){ .s = {a.s[0]-b.s[0], a.s[1]-b.s[1], a.s[2]-b.s[2]} }; 
}

void inc(Vector3 *a, const Vector3 b)
{
    (*a).s[0] += b.s[0];
    (*a).s[1] += b.s[1];
    (*a).s[2] += b.s[2];
}


Vector3 add3(const Vector3 a, const Vector3 b, const Vector3 c)
{
    return (Vector3){ .s = {a.s[0] + b.s[0] + c.s[0], 
                            a.s[1] + b.s[1] + c.s[1], 
                            a.s[2] + b.s[2] + c.s[2]} };   
}

Vector3 add4(const Vector3 a, const Vector3 b, const Vector3 c, const Vector3 d)
{
    return (Vector3){ .s = {a.s[0] + b.s[0] + c.s[0] + d.s[0], 
                            a.s[1] + b.s[1] + c.s[1] + d.s[1], 
                            a.s[2] + b.s[2] + c.s[2] + d.s[2]} };   
}


Vector3 initVector3(float x, float y, float z)
{
    return (Vector3){.s = {x, y, z}};
}


Vector3 mul(const Vector3 b, float a) { 
    return (Vector3) {.s = {b.s[0]*a, 
                            b.s[1]*a, 
                            b.s[2]*a} }; 
}

Vector3 div_vec3(const Vector3 a, float b) { 
    float rec = 1.0f/b;
    return (Vector3) {.s = {a.s[0]*rec, 
                            a.s[1]*rec,
                            a.s[2]*rec} }; 
}

Vector3 neg(const Vector3 v)
{
    return (Vector3) {.s = {-v.s[0], -v.s[1], -v.s[2]} };
}


Vector3 createVector3(float _x, float _y, float _z)
{
    return (Vector3){.s = {_x, _y, _z} };
}

Vector3 vec3(float _x, float _y, float _z)
{
    return (Vector3){.s = {_x, _y, _z} };
}

float dot(const Vector3 a, const Vector3 b)
{ 
    return a.s[0]*b.s[0] + a.s[1]*b.s[1] + a.s[2]*b.s[2];
}

Vector3 cross(const Vector3 a, const Vector3 b)      //copied from the internet, not verified
{
    return (Vector3){.s = { a.s[1]*b.s[2] - a.s[2]*b.s[1], 
                            a.s[2]*b.s[0] - a.s[0]*b.s[2], 
                            a.s[0]*b.s[1] - a.s[1]*b.s[0]} };
}

float squaredLength(const Vector3 a) 
{
    return a.s[0]*a.s[0] + a.s[1]*a.s[1] + a.s[2]*a.s[2];
}

float length(const Vector3 a) { return sqrtf( a.s[0]*a.s[0] + a.s[1]*a.s[1] + a.s[2]*a.s[2] ); }

Vector3 normalized(const Vector3 a) {
    float fac = 1.0f / length(a);
    return (Vector3){.s = {a.s[0] * fac, 
                           a.s[1] * fac, 
                           a.s[2] * fac } };
}

Vector3 getDiffuseSkyRandomRay(const Vector3 ndir/*, const Vector3 udir, const Vector3 vdir*/)
{
    //HACK: computes a lambertian quarter-sphere (lower half of hemisphere)
    
    // Step 1:Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand()/(double)RAND_MAX);
    float phi = 2 * 3.141592f * (rand()/(double)RAND_MAX);

    // Step 2: Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    if (u < 0)  //project to lower quadsphere (no light from below the horizon)
        u = -u;

    Vector3 udir = initVector3(0,0,1);
    if (fabs( dot(udir, ndir)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        udir = initVector3(0,1,0);

    Vector3 vdir = normalized( cross(udir,ndir));
    udir = normalized( cross(vdir,ndir));

    //# Convert to a direction on the hemisphere defined by the normal
    return add3( mul(udir, u), mul(vdir, v), mul(ndir, n));
}

Vector3 getCosineDistributedRandomRay(const Vector3 ndir) {
    // Step 1:Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand()/(double)RAND_MAX);
    float phi = 2 * 3.141592f * (rand()/(double)RAND_MAX);

    // Step 2: Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    
    Vector3 udir = initVector3(0,0,1);
    if (fabs( dot(udir, ndir)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        udir = initVector3(0,1,0);

    Vector3 vdir = normalized( cross(udir,ndir));
    udir = normalized( cross(vdir,ndir));

    //# Convert to a direction on the hemisphere defined by the normal
    return add3( mul(udir, u), mul(vdir, v), mul(ndir, n));
}

//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void createBase( const Vector3 ndir, Vector3 *c1, Vector3 *c2) 
{
    *c1 = vec3(0,0,1);
    if (fabs( dot(ndir, *c1)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        *c1 = vec3(0,1,0);

        
    *c2 = normalized( cross(*c1,ndir));
    *c1 = normalized( cross(*c2,ndir));

    assert( dot(ndir, *c1) < 1E6); //ensure orthogonality
    assert( dot(ndir, *c2) < 1E6);
    assert( dot(*c1, *c2) < 1E6);
    
    assert( fabsf(length(ndir) -1) < 1E6);    //ensure normality
    assert( fabsf(length(*c1) -1) < 1E6);
    assert( fabsf(length(*c2) -1) < 1E6);
    
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

