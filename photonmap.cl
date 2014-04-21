
typedef struct __attribute__ ((aligned(16))) Rectangle{
    float3 pos;
    float3 width, height;
    float3 n;
    float3 color;
    int lightBaseIdx;
} Rectangle;

//generates float random numbers in the interval [0..1]
float rand(ulong *rng_state)
{
    //printf("rng state is at 0x%08x\n", rng_state);

    //*rng_state = (*rng_state * 1664525) + 1013904223;
    //return (*rng_state) / 4294967295.0f;
    //*rng_state = (*rng_state * 6364136223846793005 + 1442695040888963407);
    //return (*rng_state) / (float)0x7FFFFFFFFFFFFFFF;
    //*rng_state = 5;
    //return 0.5;
    *rng_state = (*rng_state * 6364136223846793005 + 1);
    return (*rng_state >> 32) / (float)0xFFFFFFFF;
}

float3 getDiffuseSkyRandomRay(ulong *rng_state, float3 ndir, float3 udir, float3 vdir)
{
    //HACK: computes a lambertian quarter-sphere (lower half of hemisphere)
    
    // Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand(rng_state));
    rand(rng_state);
    float phi = 2 * 3.141592 * rand(rng_state);

    //# Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    if (v > 0)  //project to lower quadsphere (no light from below the horizon)
        v = -v;

    //# Convert to a direction on the hemisphere defined by the normal
    return udir*u + vdir*v + ndir*n;

}

int getTileIdAt(__constant const Rectangle *rect, const float3 p, const float TILE_SIZE)
{
    float3 pDir = p - rect->pos; //vector from rectangle origin (its lower left corner) to current point
    
    float hLength = length(rect->width);
    float vLength = length(rect->height);
    
    float dx = dot( rect->width / hLength, pDir);
    float dy = dot( rect->height / vLength, pDir);

    
    int hNumTiles = max( (int)round(hLength / TILE_SIZE), 1);
    int vNumTiles = max( (int)round(vLength / TILE_SIZE), 1);
    
    //FIXME: check whether a float->int conversion in OpenCL also is round-towards-zero
    int tx = clamp( (int)((dx * hNumTiles) / hLength), 0, hNumTiles);
    int ty = clamp( (int)((dy * vNumTiles) / vLength), 0, vNumTiles);
    
    //assert(ty * hNumTiles + tx < getNumTiles(rect));
    return ty * hNumTiles + tx;
}


float intersects( __constant const Rectangle *rect, float3 ray_src, float3 ray_dir, float closestDist) 
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
    
    float3 p = ray_src + ray;
    float3 pDir = p - rect->pos;
    
    /*float width_len  = length(rect->width);
    float height_len = length(rect->height);*/
    
    float widthLength = length(rect->width);
    float heightLength= length(rect->height);
    
    float dx = dot( normalize(rect->width),  pDir);
    float dy = dot( normalize(rect->height), pDir);
    
    if (dx < 0 || dy < 0|| dx > widthLength || dy > heightLength ) return -1;
    return fac;
}


__kernel void photonmap(const Rectangle window, __constant const Rectangle* rects, int numRects,
                        __global float3 *lightColors, float TILE_SIZE)
{
    //return;
    //printf("processing %d rectangles\n", numRects);
    size_t item_id = get_global_id(0);
    ulong rng_state = item_id;
    float r = rand(&rng_state) * 40; //warm-up / decorrelate individual RNGs
    for (int i = 0; i < r; i++)
        rand(&rng_state);   
        
    float3 lightColor = window.color;
    
    float dx = rand(&rng_state);
    //rand(&rng_state);
    float dy = rand(&rng_state);

    //Vector3 pos = src + xDir*dx + yDir*dy;
    float3 pos = window.pos + window.width*dx + window.height*dy;
    //float3 n   = window.n;

    float3 ray_dir = getDiffuseSkyRandomRay(&rng_state, window.n, normalize(window.width), normalize(window.height));

    pos += (ray_dir* 1E-10f); //to prevent self-intersection on the light source geometry
 
    
    /* WARNING: OpenCL objects in the 'constant' memory area on AMD hardware have their own address space
                The first object in this address space can have address 0x00000000, so a 'null' pointer 
                can indeed be valid here --> comparing 'hitObj' to 0 does not return whether hitObj points
                to a valid object */
    constant const Rectangle* hitObj = 0;
    float dist_out = INFINITY;
    
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
    //hit_obj[get_global_id(0)] = closestObject;
    float3 hit_pos = pos + ray_dir * dist_out;

    int tile_id = getTileIdAt( hitObj, hit_pos, TILE_SIZE);
    int light_idx = hitObj->lightBaseIdx + tile_id;

    //FIXME: make this increment atomic
    lightColors[ light_idx ] += 
        lightColor;
}

