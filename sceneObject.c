#include "sceneObject.h"

#include <stdlib.h> // for rand()
#include <math.h>
#include "png_helper.h"

/*
Tile::Tile(const Color3& _col): col(_col), lightColor(0,0,0) {}
Tile::Tile(): col(0.5, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX), lightColor(0,0,0) {};
    
Color3 Tile::getCombinedColor() const{ return col*lightColor;}

void   Tile::setLightColor(const Color3& color) { lightColor = color;}
Color3 Tile::getLightColor() const { return lightColor;}

void   Tile::setColor(const Color3& color) { col = color;}
Color3 Tile::getColor() const { return col;}*/

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
    res.width_norm = normalized(res.width);
    res.height_norm = normalized(res.height);
    res.hLength = length(res.width);
    res.vLength = length(res.height);
    /*r = 0.5;
    g = rand() / (float)RAND_MAX;
    b = rand() / (float)RAND_MAX;*/
    
    res.hNumTiles = max( round(length(res.width) / TILE_SIZE), 1);
    res.vNumTiles = max( round(length(res.height)/ TILE_SIZE), 1);
    
    //res.tiles = new Tile[res.vNumTiles * res.hNumTiles];
    
    res.width_per_tile = div_vec3(res.width, res.hNumTiles);
    res.height_per_tile= div_vec3(res.height, res.vNumTiles);
    //numTiles = ;
    return res;
}


Rectangle createRectangleWithColor( const Vector3 _pos, const Vector3 _width, const Vector3 _height, const Vector3 col)
{
    Rectangle res = createRectangle(_pos, _width, _height);
    /*for (int i = 0; i < res.vNumTiles * res.hNumTiles; i++)
        res.tiles[i].setColor( col);*/
    res.color = col;
        
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
    
    float dx = dot( rect->width_norm, pDir);
    float dy = dot(rect->height_norm, pDir);
    
    if (dx < 0 || dy < 0|| dx > rect->hLength || dy > rect->vLength) return -1;
    return fac;
}

/*Vector3 Rectangle::getNormalAt(const Vector3&) const {
    return n;
}*/

int getNumTiles(const Rectangle *rect)
{
    return rect->hNumTiles * rect->vNumTiles;
}

int getTileIdAt(const Rectangle *rect, const Vector3 p)
{
    Vector3 pDir = p - rect->pos; //vector from rectangle origin (its lower left corner) to current point
    float dx, dy;
    
    dx = dot(rect->width_norm, pDir);
    dy = dot(rect->height_norm,pDir);
    int tx = (dx*rect->hNumTiles) / rect->hLength;
    int ty = (dy*rect->vNumTiles) / rect->vLength;
    if (tx < 0) tx = 0;
    if (tx >= rect->hNumTiles) tx = rect->hNumTiles - 1;
    if (ty < 0) ty = 0;
    if (ty >= rect->vNumTiles) ty = rect->vNumTiles - 1;
    
    assert(ty*rect->hNumTiles + tx < getNumTiles(rect));
    return ty * rect->hNumTiles + tx;
}

/*
Tile& getTileAt(Rectangle *rect, const Vector3 &pos)
{
    return rect->tiles[getTileIdAt(rect, pos)];
}

Tile& getTile(Rectangle *rect, const int tileId)
{
    assert(tileId >= 0 && tileId < getNumTiles(rect));
    return rect->tiles[tileId];
}*/


Vector3 getTileCenter(const Rectangle *rect, int id)
{
    int ty = id / rect->hNumTiles;
    int tx = id % rect->hNumTiles;

    if (tx < 0) tx = 0;
    if (tx >= rect->hNumTiles) tx = rect->hNumTiles - 1;
    if (ty < 0) ty = 0;
    if (ty >= rect->vNumTiles) ty = rect->vNumTiles - 1;

    //return pos + width_per_tile * (tx+0.5) + height_per_tile * (ty+0.5);
    return add(rect->pos, add( mul(rect->width_per_tile, (tx+0.5)), mul( rect->height_per_tile, (ty+0.5))));
    //return pos;
}

#if 0    
Vector3 getDiffuseColor(const Rectangle *rect, const Vector3 &pos)
{ 
    return rect->color;
    //int tile_id = getTileIdAt(rect, pos);
    /*Vector3 p = getTileCenter(tile_id);
    
    float dist = (pos - p).length();
    //cout << dist << endl;
    float fac = (1-dist/7.0);*/
    //return rect->tiles[tile_id].getCombinedColor();
    //return Color3(r,g,b); 

}
#endif
    
/*void Rectangle::setTileColor(const int tileId, const Color3 &color) {
    assert (tileId >= 0 && tileId < hNumTiles*vNumTiles);
    tiles[tileId].setLightColor(color);
}*/

static uint8_t clamp(float d)
{
    if (d < 0) d = 0;
    if (d > 255) d = 255;
    return d;
}

//convert light energy to perceived brightness
float convert(float color)
{
    return 1 - exp(-color);
}

void saveAs(const Rectangle *rect, const char *filename, Vector3 *lights)
{
    uint8_t *data = (uint8_t*) malloc( rect->hNumTiles * rect->vNumTiles * 3 * sizeof(uint8_t));
    for (int i = 0; i < rect->hNumTiles * rect->vNumTiles; i++)
    {
            /*col.r = 1 - exp(-col.r);
            col.g = 1 - exp(-col.g);
            col.b = 1 - exp(-col.b);*/
        
        data[i*3+0] = clamp( convert(rect->color[0] * lights[rect->lightBaseIdx + i][0])*255);
        data[i*3+1] = clamp( convert(rect->color[1] * lights[rect->lightBaseIdx + i][1])*255);
        data[i*3+2] = clamp( convert(rect->color[2] * lights[rect->lightBaseIdx + i][2])*255);
    }
    
    write_png_file(filename, rect->hNumTiles, rect->vNumTiles, PNG_COLOR_TYPE_RGB, data);
    free (data);
}

Vector3 getOrigin(const Rectangle *rect) { return rect->pos; }
Vector3 getWidthVector(const Rectangle *rect) { return rect->width; }
Vector3 getHeightVector(const Rectangle *rect) { return rect->height;}

