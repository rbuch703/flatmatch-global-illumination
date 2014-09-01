
typedef struct __attribute__ ((aligned(16))) Rectangle {
    float3 pos;
    float3 width, height;
    float3 n;
//    float3 color;
    int3 lightmapSetup;
//    int lightNumTiles;
} Rectangle;

float rand(uint *rng_state);
float3 getDiffuseSkyRandomRay(uint *rng_state, const float3 ndir/*, const float3 udir, const float3 vdir*/);
float3 getCosineDistributedRandomRay(uint *rng_state, const float3 ndir);
int getTileIdAt(__constant const Rectangle *rect, const float3 p);
float intersects( __constant const Rectangle *rect, const float3 ray_src, const float3 ray_dir, const float closestDist);
//void tracePhoton(uint *rng_state, __constant const Rectangle *window, __constant const Rectangle* rects, const int numRects, __global float3 *lightColors, /*const int numLightColors,*/ const float TILE_SIZE);



//generates float random numbers in the interval [0..1]
float rand(uint *rng_state)
{
    *rng_state = (*rng_state * 1664525 + 1013904223);
    return (*rng_state) / (float)0xFFFFFFFF;
}

float3 getDiffuseSkyRandomRay(uint *rng_state, const float3 ndir/*, const float3 udir, const float3 vdir*/)
{
    //HACK: computes a lambertian quarter-sphere (lower half of hemisphere)
    
    // Step 1:Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand(rng_state));
    float phi = 2 * 3.141592f * rand(rng_state);

    // Step 2: Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    if (u < 0)  //project to lower quadsphere (no light from below the horizon)
        u = -u;

    float3 udir = (float3)(0,0,1);
    if (fabs( dot(udir, ndir)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        udir = (float3)(0,1,0);

    float3 vdir = normalize( cross(udir,ndir));
    udir = normalize( cross(vdir,ndir));

    //# Convert to a direction on the hemisphere defined by the normal
    return udir*u + vdir*v + ndir*n;
}

float3 getCosineDistributedRandomRay(uint *rng_state, const float3 ndir) {
    // Step 1:Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand(rng_state));
    float phi = 2 * 3.141592f * rand(rng_state);

    // Step 2: Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    
    float3 udir = (float3)(0,0,1);
    if (fabs( dot(udir, ndir)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        udir = (float3)(0,1,0);

    float3 vdir = normalize( cross(udir,ndir));
    udir = normalize( cross(vdir,ndir));

    //# Convert to a direction on the hemisphere defined by the normal
    return udir*u + vdir*v + ndir*n;
}

#if 0
//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void createBase( const float3 ndir, float3 *c1, float3 *c2) {
    *c1 = (float3)(0,0,1);
    //printf("c1: (%f, %f, %f)\n", (*c1).s0, (*c1).s1, (*c1).s2);
    if (fabs( dot(*c1, ndir)) == 1) //are colinear --> cannot build coordinate base
        *c1 = (float3)(0,1,0);

    //printf("c1: (%f, %f, %f)\n", (*c1).s0, (*c1).s1, (*c1).s2);
        
    *c2 = normalize( cross(*c1,ndir));
    *c1 = normalize( cross(*c2,ndir));
    /*printf("n : (%f, %f, %f)\n", ndir.s0,  ndir.s1,  ndir.s2);
    printf("c1: (%f, %f, %f)\n", (*c1).s0, (*c1).s1, (*c1).s2);
    printf("c2: (%f, %f, %f)\n", (*c2).s0, (*c2).s1, (*c2).s2);*/
}
#endif


int getTileIdAt(__constant const Rectangle *rect, const float3 p)
{
    float3 pDir = p - rect->pos; //vector from rectangle origin (its lower left corner) to current point
    
    float hLength = length(rect->width);
    float vLength = length(rect->height);
    
    float dx = dot( rect->width / hLength, pDir);
    float dy = dot( rect->height / vLength, pDir);

    
    int hNumTiles = rect->lightmapSetup.s1;//max( (int)ceil(hLength / TILE_SIZE), 1);
    int vNumTiles = rect->lightmapSetup.s2;//max( (int)ceil(vLength / TILE_SIZE), 1);
    //printf("rectangle has %dx%d tiles\n", hNumTiles, vNumTiles);
    //FIXME: check whether a float->int conversion in OpenCL also is round-towards-zero
    int tx = clamp( (int)(dx * hNumTiles / hLength), 0, hNumTiles-1);
    int ty = clamp( (int)(dy * vNumTiles / vLength), 0, vNumTiles-1);
    
    /*if (ty * hNumTiles + tx >= rect->lightNumTiles)
    {
        printf("Invalid tile index %d in rect %#x\n", ty * hNumTiles + tx, rect);
        return 0;
    }*/
    //assert(ty * hNumTiles + tx < getNumTiles(rect));
    return ty * hNumTiles + tx;
}


float intersects( __constant const Rectangle *rect, const float3 ray_src, const float3 ray_dir, const float closestDist) 
{
    //if (dot(ray_dir,n) > 0) return -1; //backface culling
    float denom = dot(rect->n, ray_dir);
    if (denom >= 0) // == 0 > ray lies on plane; >0 --> is a backface
        return -1;
        
    //float fac = n.dot( pos - ray_src ) / denom;
    float fac = dot(rect->n, rect->pos - ray_src) / denom;
    if (fac < 0) 
        return -1;    //is behind camera, cannot be hit
    
    float3 ray = ray_dir * fac;
    
    //early termination: if further away than the closest hit (so far), we can ignore this hit
    //N.B.: dot(a,a) = squaredLength(a);
    if (closestDist * closestDist < dot(ray, ray) )
        return -1;
    
    float3 pDir = (ray_src + ray) - rect->pos;
    
    float widthLength = length(rect->width);
    float dx = dot( rect->width / widthLength,  pDir);
    if (dx < 0 || dx > widthLength)
        return -1;
        
    float heightLength= length(rect->height);
    float dy = dot( rect->height/ heightLength, pDir);
    if  ( dy < 0 || dy > heightLength )
        return -1;
        
    return fac;

    //return select(-1.0f, fac,dx == clamp(dx, 0.0f, widthLength) && dy == clamp(dy, 0.0f, heightLength));

}


void tracePhoton(uint *rng_state, __constant const Rectangle *window, __constant const Rectangle* rects, const int numRects,
                        __global float3 *lightColors, const int isWindow)
{

    
    //float3 lightColor = 
    float3 lightColor = isWindow ? 
        (float3)(18, 18, 18):  //slightly yellow-ish for outside areas
        (float3)(16, 16, 18);  //slightly blue-ish for artificial light

    const int MAX_DEPTH = 8;

    float dx = rand(rng_state);
    float dy = rand(rng_state);

    //float3 ray_dir = getDiffuseSkyRandomRay(rng_state, window->n);//, normalize(window.width), normalize(window.height));
    float3 ray_dir = isWindow ? 
        getDiffuseSkyRandomRay(rng_state, window->n) :
        getCosineDistributedRandomRay(rng_state, window->n);
    //move slightly in direction of ray_dir to prevent self-intersection on the light source geometry
    float3 pos = window->pos + window->width*dx + window->height*dy + (ray_dir* 1E-5f);
    
    for (int depth = 0; depth < MAX_DEPTH; depth++)
    {
        /* WARNING: OpenCL objects in the 'constant' memory area on AMD hardware have their own address space
                    The first object in this address space can have address 0x00000000, so a 'null' pointer 
                    can indeed be valid here --> comparing 'hitObj' to 0 does not return whether hitObj points
                    to a valid object */
        const constant Rectangle* hitObj = 0;
        float dist_out = INFINITY;

        //printf("work_item %d, pos (%f,%f,%f), dir (%f,%f,%f) \n", get_global_id(0), pos.s0, pos.s1, pos.s2, ray_dir.s0, ray_dir.s1, ray_dir.s2);
        
        for ( int i = 0; i < numRects; i++)
        {

            constant const Rectangle *target = &(rects[i]);
            float dist = intersects(target , pos, ray_dir, dist_out);
            if (dist < 0)
                continue;
                
            if (dist < dist_out) {
                hitObj = target;
                dist_out = dist;
            }
        }
        
        if (dist_out == INFINITY)
            return;
            
        //if ( hitObj->color.x > 1.0f || hitObj->color.y > 1.0f || hitObj->color.z > 1.0f)
        //    return; //hit a light source;
            
        //hit_obj[get_global_id(0)] = closestObject;
        //re-set the ray position to the intersection point (which is the starting position for the next ray);
        pos = pos + ray_dir * dist_out;
        
        int tile_id = getTileIdAt( hitObj, pos);
        int light_idx = hitObj->lightmapSetup.s0 + tile_id;
        
        /*if (light_idx >= numLightColors)
        {
            printf("invalid light index %d\n", light_idx);
            return;
        }*/
        
        //hack: make floor slightly brownish
        if (pos.s2 < 1E-5f)
        {
            lightColor.s0 *= 1.0f;
            lightColor.s1 *= 0.9f;
            lightColor.s2 *= 0.8f;
        }    
        //FIXME: make this increment atomic
        lightColors[ light_idx ] += 
            lightColor;
            
        lightColor *= /*(hitObj->color**/0.9f;
        //printf ("lightColor %d/%d is (%f, %f, %f)\n", get_global_id(0), depth, lightColor.x, lightColor.y, lightColor.z);
        
        //pos = hit_pos;
        //float3 udir, vdir;
        //createBase(hitObj->n, &udir, &vdir);
        //printf("work_item %d, base1 (%f,%f,%f), base2 (%f,%f,%f) \n", get_global_id(0), udir.s0, udir.s1, udir.s2, vdir.s0, vdir.s1, vdir.s2);
        //printf("work_item %d, hit_pos (%f,%f,%f), new_dir (%f,%f,%f) \n", get_global_id(0), pos.s0, pos.s1, pos.s2, ray_dir.s0, ray_dir.s1, ray_dir.s2);
        ray_dir = getCosineDistributedRandomRay(rng_state, hitObj->n);//, udir, vdir);
        //ray_dir = getCosineDistributedRandomRay(rng_state, hitObj->n, normalize(hitObj->width), normalize(hitObj->height));
        pos += (ray_dir* 1E-5f); //to prevent self-intersection on the light source geometry
            
    }
}



__kernel void photonmap(__constant const Rectangle * restrict window, __constant const Rectangle* restrict rects, int numRects,
                        __global float3 *lightColors/*, const int numLightColors*/, int rng_offset, int isWindow)
{
    uint rng_state = get_global_id(0) + rng_offset;
    float r = rand(&rng_state) * 40; //warm-up / decorrelate individual RNGs
    for (int i = 0; i < r; i++)
        rand(&rng_state);   
    
    //printf("kernel supplied with %d rectangles\n", numRects);
    
    for (int i = 0; i < 100; i++)
        tracePhoton(&rng_state, window, rects, numRects, lightColors, isWindow);
}

