
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
                        __global int *hit_obj, __global float3 *hit_pos)
{
    //printf("processing %d rectangles\n", numRects);
    size_t item_id = get_global_id(0);
    ulong rng_state = item_id;
    float r = rand(&rng_state) * 40; //warm-up / decorrelate individual RNGs
    for (int i = 0; i < r; i++)
        rand(&rng_state);   
    
    float dx = rand(&rng_state);
    //rand(&rng_state);
    float dy = rand(&rng_state);

    //Vector3 pos = src + xDir*dx + yDir*dy;
    float3 pos = window.pos + window.width*dx + window.height*dy;
    //float3 n   = window.n;

    /*printf("rectangle position: (%f, %f, %f)\n", window.pos.s0, window.pos.s1, window.pos.s2);
    printf("rectangle width: (%f, %f, %f)\n", window.width.s0, window.width.s1, window.width.s2);
    printf("rectangle height: (%f, %f, %f)\n", window.height.s0, window.height.s1, window.height.s2);
    printf("rectangle n: (%f, %f, %f)\n", window.n.s0, window.n.s1, window.n.s2);
    printf("rectangle color: (%f, %f, %f)\n", window.color.s0, window.color.s1, window.color.s2);
    printf("baseIdx: %d\n\n", window.lightBaseIdx);*/
    //printf("rng: %d\n", rng_state);
    float3 ray_dir = getDiffuseSkyRandomRay(&rng_state, window.n, normalize(window.width), normalize(window.height));

    pos += (ray_dir* 1E-10f); //to prevent self-intersection on the light source geometry
    //ray_src[i] = pos;
    
    
    //printf("item_id: %d, ray_src: (%f, %f, %f), ray_dir: (%f, %f, %f), hit_obj: %d, hit_dist: %f\n", item_id, ray_src[item_id].s0, ray_src[item_id].s1, ray_src[item_id].s2, ray_dir[item_id].s0, ray_dir[item_id].s1, ray_dir[item_id].s2, hit_obj[item_id], hit_dist[item_id]);
//int numObjects = objects.size();

    int closestObject = -1;
    float dist_out = INFINITY;
    
    for ( int i = 0; i < numRects; i++)
    {
        //printf("iteration %d\n", i);
        /*
        printf("rectangle position: (%f, %f, %f)\n", rects[i].pos.s0, rects[i].pos.s1, rects[i].pos.s2);
        printf("rectangle width: (%f, %f, %f)\n", rects[i].width.s0, rects[i].width.s1, rects[i].width.s2);
        printf("rectangle height: (%f, %f, %f)\n", rects[i].height.s0, rects[i].height.s1, rects[i].height.s2);
        printf("rectangle n: (%f, %f, %f)\n", rects[i].n.s0, rects[i].n.s1, rects[i].n.s2);
        printf("rectangle color: (%f, %f, %f)\n", rects[i].color.s0, rects[i].color.s1, rects[i].color.s2);
        printf("baseIdx: %d\n\n", rects[i].lightBaseIdx);*/

        float dist = intersects( &(rects[i]), pos, ray_dir, dist_out);
        if (dist < 0)
            continue;
            
        if (dist < dist_out) {
            closestObject = i;
            dist_out = dist;
        }
    }
    
    hit_obj[get_global_id(0)] = closestObject;
    hit_pos[get_global_id(0)] = pos + ray_dir * dist_out;
    
    //return closestObject;
}

