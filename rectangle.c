#include "rectangle.h"

//#include <stdlib.h> // for rand()
//#include <math.h>
#include "png_helper.h"

#include "imageProcessing.h"


int max(int a, int b)
{
    return a > b ? a : b;
}


Rectangle createRectangle( const Vector3 _pos, const Vector3 _width, const Vector3 _height)
{

    Rectangle res;
    res.pos = _pos;
    res.width= _width;
    res.height= _height;
    res.n = normalized(cross(res.height,res.width));
    res.lightmapSetup.s[0] = 0;    //invalidated, should be set through main()
    res.lightmapSetup.s[1] = 1;   // texture width
    res.lightmapSetup.s[2] = 1;   // texture height;
    res.lightmapSetup.s[3] = 0; //unused
    
    float width = length(_width);
    float height = length(_height);
    float tile_size = (res.lightmapSetup.s[1] * res.lightmapSetup.s[2]) / (width*height);
    while (tile_size < TILE_SIZE) 
    {
        float width_res = res.lightmapSetup.s[1] / width;
        float height_res= res.lightmapSetup.s[2] / height;
        
        if (width_res < height_res)
            res.lightmapSetup.s[1] *= 2;
        else 
            res.lightmapSetup.s[2] *= 2;

        tile_size = (res.lightmapSetup.s[1] * res.lightmapSetup.s[2]) / (width*height);
    }
    
    res.lightmapSetup.s[1] *= SUPER_SAMPLING;
    res.lightmapSetup.s[2] *= SUPER_SAMPLING;
    
    
    //HACK: cl_float3 is actually a float[4] to ensure 16 bytes alignment
    //      use the extraneous float to store the vector length
    //      (in effect this make the vector a unit vector in homogenous coordinates)
    /*res.width.s[3] = length(res.width);
    res.height.s[3]= length(res.height);*/
    /*res.width_norm = ;
    res.height_norm = ;*/

    return res;
}


float intersects( const Rectangle *rect, Vector3 ray_src, Vector3 ray_dir, float closestDist) 
{
    //if (dot(ray_dir,n) > 0) return -1; //backface culling
    float denom = dot(rect->n, ray_dir);
    if (denom >= 0) // == 0 > ray lies on plane; >0 --> is a backface
        return -1;
        
    //float fac = n.dot( pos - ray_src ) / denom;
    float fac = dot(rect->n, sub(rect->pos, ray_src)) / denom;
    if (fac < 0) 
        return -1;    //is behind camera, cannot be hit
    
    Vector3 ray = mul(ray_dir, fac);
    
    //early termination: if further away than the closest hit (so far), we can ignore this hit
    if (closestDist * closestDist < squaredLength(ray) )
        return -1;
    
    Vector3 p = add(ray_src, ray);
    Vector3 pDir = sub(p, rect->pos);
    
    float width_len  = length(rect->width);
    float height_len = length(rect->height);
    
    float dx = dot( div_vec3( rect->width, width_len), pDir);
    float dy = dot( div_vec3( rect->height, height_len), pDir);
    
    if (dx < 0 || dy < 0|| dx > width_len || dy > height_len ) return -1;
    return fac;
}

/*Vector3 Rectangle::getNormalAt(const Vector3&) const {
    return n;
}*/

int getNumTiles(const Rectangle *rect)
{
    
    //int hNumTiles = max( ceil(length(rect->width) / TILE_SIZE), 1);
    //int vNumTiles = max( ceil(length(rect->height)/ TILE_SIZE), 1);

    return rect->lightmapSetup.s[1] * rect->lightmapSetup.s[2];//hNumTiles * vNumTiles;
}


float getArea(const Rectangle *rect)
{
    return length(rect->width) * length(rect->height);
}

/*
int getTileIdAt(const Rectangle *rect, const Vector3 p)
{
    Vector3 pDir = sub(p,rect->pos); //vector from rectangle origin (its lower left corner) to current point
    //float dx, dy;
    
    float width_len  = length(rect->width);
    float height_len = length(rect->height);
    
    float dx = dot( div_vec3( rect->width, width_len), pDir);
    float dy = dot( div_vec3( rect->height, height_len), pDir);

    float hLength = length(rect->width);
    float vLength = length(rect->height);
    
    int hNumTiles = max( ceil(hLength / TILE_SIZE), 1);
    int vNumTiles = max( ceil(vLength / TILE_SIZE), 1);
    
    int tx = (dx * hNumTiles) / hLength;
    int ty = (dy * vNumTiles) / vLength;
    if (tx < 0) tx = 0;
    if (tx >= hNumTiles) tx = hNumTiles - 1;
    if (ty < 0) ty = 0;
    if (ty >= vNumTiles) ty = vNumTiles - 1;
    
    assert(ty * hNumTiles + tx < getNumTiles(rect));
    return ty * hNumTiles + tx;
}
*/


void saveAs(const Rectangle *rect, const char *filename, Vector3 *lights)
{
    int baseIdx = rect->lightmapSetup.s[0];
    int hNumTiles = rect->lightmapSetup.s[1];//max( ceil(length(rect->width) / TILE_SIZE), 1);
    int vNumTiles = rect->lightmapSetup.s[2];//max( ceil(length(rect->height)/ TILE_SIZE), 1);
    assert( hNumTiles % SUPER_SAMPLING == 0);
    assert( vNumTiles % SUPER_SAMPLING == 0);
    hNumTiles /= SUPER_SAMPLING;
    vNumTiles /= SUPER_SAMPLING;

    uint8_t *data = (uint8_t*) malloc( hNumTiles * vNumTiles * 3 * sizeof(uint8_t));
    
    subsampleAndConvertToPerceptive( lights+baseIdx, data, hNumTiles, vNumTiles);
    selectiveDilate(data, hNumTiles, vNumTiles);

#if 0    
    for (int i = 0; i < hNumTiles * vNumTiles; i++)
    {
            /*col.r = 1 - exp(-col.r);
            col.g = 1 - exp(-col.g);
            col.b = 1 - exp(-col.b);*/
        
        data[i*3+0] = clamp( convert(lights[baseIdx + i].s[0])*255);
        data[i*3+1] = clamp( convert(lights[baseIdx + i].s[1])*255);
        data[i*3+2] = clamp( convert(lights[baseIdx + i].s[2])*255);
    }
#endif
    write_png_file(filename, hNumTiles, vNumTiles, PNG_COLOR_TYPE_RGB, data);
    free (data);
}

Vector3 getOrigin(const Rectangle *rect) { return rect->pos; }
Vector3 getWidthVector(const Rectangle *rect) { return rect->width; }
Vector3 getHeightVector(const Rectangle *rect) { return rect->height;}

