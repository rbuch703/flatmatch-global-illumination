
typedef struct __attribute__ ((aligned(16))) Rectangle{
    float3 pos;
    float3 width, height;
    float3 n;
    float3 color;
    int lightBaseIdx;
} Rectangle;

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


__kernel void photonmap(__global float3 *ray_src, __global float3 *ray_dir, __constant const Rectangle* rects, int numRects,
                        __global int *hit_obj, __global float *hit_dist )
{
    //printf("processing %d rectangles\n", numRects);
    int closestObject = -1;
    float dist_out = INFINITY;
    size_t item_id = get_global_id(0);
    //printf("item_id: %d, ray_src: (%f, %f, %f), ray_dir: (%f, %f, %f), hit_obj: %d, hit_dist: %f\n", item_id, ray_src[item_id].s0, ray_src[item_id].s1, ray_src[item_id].s2, ray_dir[item_id].s0, ray_dir[item_id].s1, ray_dir[item_id].s2, hit_obj[item_id], hit_dist[item_id]);
//int numObjects = objects.size();
    
    for ( int i = 0; i < numRects; i++)
    {
        /*printf("rectangle position: (%f, %f, %f)\n", rects[i].pos.s0, rects[i].pos.s1, rects[i].pos.s2);
        printf("rectangle width: (%f, %f, %f)\n", rects[i].width.s0, rects[i].width.s1, rects[i].width.s2);
        printf("rectangle height: (%f, %f, %f)\n", rects[i].height.s0, rects[i].height.s1, rects[i].height.s2);
        printf("rectangle n: (%f, %f, %f)\n", rects[i].n.s0, rects[i].n.s1, rects[i].n.s2);
        printf("rectangle color: (%f, %f, %f)\n", rects[i].color.s0, rects[i].color.s1, rects[i].color.s2);
        printf("baseIdx: %d\n\n", rects[i].lightBaseIdx);*/

        float dist = intersects( &(rects[i]), ray_src[get_global_id(0)], ray_dir[get_global_id(0)], dist_out);
        if (dist < 0)
            continue;
            
        if (dist < dist_out) {
            closestObject = i;
            dist_out = dist;
        }
    }
    
    hit_obj[get_global_id(0)] = closestObject;
    hit_dist[get_global_id(0)] = dist_out;
    //return closestObject;
}

