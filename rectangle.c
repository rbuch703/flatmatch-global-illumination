#include "rectangle.h"

//#include <stdlib.h> // for rand()
//#include <math.h>
#include "png_helper.h"

//#include "imageProcessing.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int max(int a, int b)
{
    return a > b ? a : b;
}


Rectangle createRectangleV( const Vector3 _pos, const Vector3 _width, const Vector3 _height)
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
    float tile_size = ((float)res.lightmapSetup.s[1] * res.lightmapSetup.s[2]) / (width*height);
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

Rectangle createRectangle( float px, float py, float pz,
                           float wx, float wy, float wz,
                           float hx, float hy, float hz)
{
    return createRectangleV (createVector3(px, py, pz), createVector3(wx, wy, wz), createVector3(hx, hy, hz));
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

float distanceToPlane( Vector3 planeNormal, Vector3 planePos, Vector3 ray_src, Vector3 ray_dir)
{
    //if (dot(ray_dir,n) > 0) return -1; //backface culling
    float denom = dot(planeNormal, ray_dir);
    if (denom == 0) // == 0 > ray lies on plane or is parallel to it
        return -1;
        
    //float fac = n.dot( pos - ray_src ) / denom;
    float fac = dot(planeNormal, sub(planePos, ray_src)) / denom;
    if (fac < 0) 
        return -1;    //is behind camera, cannot be hit
    
    return fac;
}

/*Vector3 Rectangle::getNormalAt(const Vector3&) const {
    return n;
}*/

Vector3 getTileCenter(const Rectangle *rect, int tileId)
{
    if (tileId  >= getNumTiles(rect)) return vec3(0,0,0);
    
    Vector3 vWidth = div_vec3( rect->width,  rect->lightmapSetup.s[1]);
    Vector3 vHeight= div_vec3( rect->height, rect->lightmapSetup.s[2]);
  
    int tx = tileId % rect->lightmapSetup.s[1];
    int ty = tileId / rect->lightmapSetup.s[1];
    
    return add3( rect->pos, mul(vWidth, tx+0.5), mul(vHeight, ty+0.5) );
//        return ty * hNumTiles + tx;

    
}


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
static int clamp_int(int val, int lo, int hi)
{
    return val < lo ? lo : (val > hi ? hi : val);
}


int getTileIdAt(const Rectangle *rect, const Vector3 p)
{
    Vector3 pDir = sub(p, rect->pos); //vector from rectangle origin (its lower left corner) to current point
    
    float hLength = length(rect->width);
    float vLength = length(rect->height);
    
    float dx = dot( div_vec3(rect->width, hLength), pDir);
    float dy = dot( div_vec3(rect->height, vLength), pDir);

    
    int hNumTiles = rect->lightmapSetup.s[1];//max( (int)ceil(hLength / TILE_SIZE), 1);
    int vNumTiles = rect->lightmapSetup.s[2];//max( (int)ceil(vLength / TILE_SIZE), 1);
    //printf("rectangle has %dx%d tiles\n", hNumTiles, vNumTiles);
    //FIXME: check whether a float->int conversion in OpenCL also is round-towards-zero
    int tx = clamp_int( (int)(dx * hNumTiles / hLength), 0, hNumTiles-1);
    int ty = clamp_int( (int)(dy * vNumTiles / vLength), 0, vNumTiles-1);
    
    /*if (ty * hNumTiles + tx >= rect->lightNumTiles)
    {
        printf("Invalid tile index %d in rect %#x\n", ty * hNumTiles + tx, rect);
        return 0;
    }*/
    //assert(ty * hNumTiles + tx < getNumTiles(rect));
    return ty * hNumTiles + tx;
}


//convert light energy to perceived brightness
float convert(float color)
{
//    if (color < 0.8)
//        return 1 - exp(-2*color);
//    else
//        return 1/16.0*color + 0.75;
    return 1 - exp(-1.5*color);
    //return 5/8.0* pow(color, 1/3.0);
}


void convert2(float *r, float *g, float *b)
{
    
    float luminance = 0.2126 * (*r) + 0.7152* (*g) + 0.0722 * (*b);
    
    float luminance_perceptive = convert(luminance);
     
    (*r) *= luminance_perceptive/luminance;
    (*g) *= luminance_perceptive/luminance;
    (*b) *= luminance_perceptive/luminance;
}

static uint8_t clamp(float d)
{
    if (d < 0) d = 0;
    if (d > 255) d = 255;
    return d;
}


void saveAs(const Rectangle *rect, const char *filename, const Vector3 *lights)
{
    int baseIdx = rect->lightmapSetup.s[0];
    int hNumTiles = rect->lightmapSetup.s[1];//max( ceil(length(rect->width) / TILE_SIZE), 1);
    int vNumTiles = rect->lightmapSetup.s[2];//max( ceil(length(rect->height)/ TILE_SIZE), 1);

    uint8_t *data = (uint8_t*) malloc( hNumTiles * vNumTiles * 3 * sizeof(uint8_t));
    
    for (int i = 0; i < hNumTiles*vNumTiles; i++)
    {
        Vector3 color = (lights[baseIdx + i]);
        convert2( &(color.s[0]), &(color.s[1]), &(color.s[2]) );
        data[3*i+0] = clamp( color.s[0] * 255 );
        data[3*i+1] = clamp( color.s[1] * 255 );
        data[3*i+2] = clamp( color.s[2] * 255 );
    }
        
    /*
    subsampleAndConvertToPerceptive( lights+baseIdx, data, hNumTiles, vNumTiles);
    selectiveDilate(data, hNumTiles, vNumTiles);*/

    /*hack: make floor slightly brownish *after* the global illumination and thus
     *      after its brown color would cause too much color bleeding;
     */
    if (rect->pos.s[2] == 0 && rect->width.s[2] == 0 && rect->height.s[2] == 0)
    {
        for (int i = 0; i < hNumTiles * vNumTiles *3; i+=3)
        {
            //data[i+0]
            data[i+1] *= 0.95;
            data[i+2] *= 0.9;
            
            data[i+0] *= 1.0f;
            data[i+1] *= 0.85f*0.95;
            data[i+2] *= 0.7f*0.9;
        }
    }
    
    write_png_file(filename, hNumTiles, vNumTiles, PNG_COLOR_TYPE_RGB, data);
    free (data);
}

typedef struct {
    int pixelWidth;
    int pixelHeight;
    float pos[3];
    float width[3];
    float height[3];
} TileMetadata;

void saveAsRaw(const Rectangle *rect, const char *filename, const Vector3 *lights)
{
    int baseIdx = rect->lightmapSetup.s[0];
    int hNumTiles = rect->lightmapSetup.s[1];//max( ceil(length(rect->width) / TILE_SIZE), 1);
    int vNumTiles = rect->lightmapSetup.s[2];//max( ceil(length(rect->height)/ TILE_SIZE), 1);

    float *data = (float*) malloc( hNumTiles * vNumTiles * 3 * sizeof(float));
    
    for (int i = 0; i < hNumTiles*vNumTiles; i++)
    {
        Vector3 color = (lights[baseIdx + i]);
        data[3*i+0] = color.s[0];
        data[3*i+1] = color.s[1];
        data[3*i+2] = color.s[2];
    }
    
    TileMetadata header ={  .pixelWidth = hNumTiles, 
                            .pixelHeight = vNumTiles,
                            .pos = {rect->pos.s[0],   rect->pos.s[1],   rect->pos.s[2]  },
                            .width={rect->width.s[0], rect->width.s[1], rect->width.s[2]},
                            .height={rect->height.s[0], rect->height.s[1], rect->height.s[2]}
                         };
    
    
    FILE* f = fopen(filename, "wb");
    fwrite(&header, sizeof(header), 1, f);
    fwrite(data, hNumTiles * vNumTiles * 3 * sizeof(float), 1, f);
    fclose(f);
   
    free (data);
}


Vector3 getOrigin(const Rectangle *rect) { return rect->pos; }
Vector3 getWidthVector(const Rectangle *rect) { return rect->width; }
Vector3 getHeightVector(const Rectangle *rect) { return rect->height;}

double getDistance(const Rectangle *plane, const Vector3 p)
{
    Vector3 dir = sub(p, plane->pos);

    return dot(dir, plane->n);
}

int getPosition(const Rectangle *plane, const Rectangle *rect)
{
    Vector3 p1 = rect->pos;
    Vector3 p2 = add( rect->pos, rect->width);
    Vector3 p3 = add( rect->pos, rect->height);
    Vector3 p4 = add3(rect->pos, rect->width, rect->height);

    int isLeft = 0;
    int isRight= 0;
    
    double d = getDistance(plane, p1);
    isLeft |= (d < 0);
    isRight|= (d > 0);

    d = getDistance(plane, p2);
    isLeft |= (d < 0);
    isRight|= (d > 0);
    
    d = getDistance(plane, p3);
    isLeft |= (d < 0);
    isRight|= (d > 0);
    
    d = getDistance(plane, p4);
    isLeft |= (d < 0);
    isRight|= (d > 0);
    
    if (isLeft && !isRight) return -1;
    if (isRight&& !isLeft) return 1;
    
    return 0;    
}

Geometry* createGeometryObject() 
{
    return (Geometry*)malloc(sizeof(Geometry));
}

