#include "sceneObject.h"

#include <stdlib.h> // for rand()
#include "png_helper.h"


Tile::Tile(const Color3& _col): col(_col), lightColor(0,0,0) {}
Tile::Tile(): col(0.5, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX), lightColor(0,0,0) {};
    
Color3 Tile::getCombinedColor() const{ return col*lightColor;}

void   Tile::setLightColor(const Color3& color) { lightColor = color;}
Color3 Tile::getLightColor() const { return lightColor;}

void   Tile::setColor(const Color3& color) { col = color;}
Color3 Tile::getColor() const { return col;}


Rectangle::Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height):
        pos(_pos), width(_width), height(_height), 
        n(normalized(cross(height,width))),
        width_norm (normalized(width)),
        height_norm (normalized(height)),
        hLength( length(width)),
        vLength( length(height)) {
    /*r = 0.5;
    g = rand() / (float)RAND_MAX;
    b = rand() / (float)RAND_MAX;*/
    
    hNumTiles = round(length(width) / TILE_SIZE);
    if (hNumTiles < 1) hNumTiles = 1;
    
    vNumTiles = round(length(height)/ TILE_SIZE);
    if (vNumTiles < 1) vNumTiles = 1;
    
    tiles = new Tile[vNumTiles * hNumTiles];
    
    width_per_tile = div(width, hNumTiles);
    height_per_tile= div(height, vNumTiles);
    //numTiles = ;
}


Rectangle::Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height, const Color3 &col):  Rectangle(_pos, _width, _height)
{
    for (int i = 0; i < vNumTiles * hNumTiles; i++)
        tiles[i].setColor( col);
}
#if 0
        pos(_pos), width(_width), height(_height), 
        n(normalized(height.cross(width))),
        width_norm (normalized(width)),
        height_norm (normalized(height)),
        hLength( width.length()),
        vLength(height.length()) {
    /*r = 0.5;
    g = rand() / (float)RAND_MAX;
    b = rand() / (float)RAND_MAX;*/
    
    hNumTiles = round(width.length() / TILE_SIZE);
    if (hNumTiles < 1) hNumTiles = 1;
    
    vNumTiles = round(height.length()/ TILE_SIZE);
    if (vNumTiles < 1) vNumTiles = 1;
    
    tiles = new Tile[vNumTiles * hNumTiles];
    
    width_per_tile = width / hNumTiles;
    height_per_tile= height/ vNumTiles;
    //numTiles = ;
}
#endif
float Rectangle::intersects( Vector3 ray_src, Vector3 ray_dir, float closestDist) {
    //if (dot(ray_dir,n) > 0) return -1; //backface culling
    float denom = dot(n, ray_dir);
    if (denom >= 0) // == 0 > ray lies on plane; >0 --> is a backface
        return -1;
        
    //float fac = n.dot( pos - ray_src ) / denom;
    float fac = dot(n, sub(pos, ray_src)) / denom;
    if (fac < 0) 
        return -1;    //is behind camera, cannot be hit
    
    Vector3 ray = mul(ray_dir, fac);
    
    //early termination: if further away than the closest hit (so far), we can ignore this hit
    if (closestDist * closestDist < squaredLength(ray) )
        return -1;
    
    Vector3 p = add(ray_src, ray);
    Vector3 pDir = sub(p, pos);
    
    float dx = dot( width_norm, pDir);
    float dy = dot(height_norm, pDir);
    
    if (dx < 0 || dy < 0|| dx > hLength || dy > vLength) return -1;
    return fac;
    
    
}

Vector3 Rectangle::getNormalAt(const Vector3&) const {
    return n;
}

int Rectangle::getNumTiles() const
{
    return hNumTiles * vNumTiles;
}

int Rectangle::getTileIdAt(const Vector3 &p) const
{
    Vector3 pDir = p - pos; //vector from rectangle origin (its lower left corner) to current point
    float dx, dy;
    
    dx = dot(width_norm, pDir);
    dy = dot(height_norm,pDir);
    int tx = (dx*hNumTiles) / hLength;
    int ty = (dy*vNumTiles) / vLength;
    if (tx < 0) tx = 0;
    if (tx >= hNumTiles) tx = hNumTiles - 1;
    if (ty < 0) ty = 0;
    if (ty >= vNumTiles) ty = vNumTiles - 1;
    
    assert(ty*hNumTiles + tx < getNumTiles());
    return ty*hNumTiles + tx;
}

Tile& Rectangle::getTile(const Vector3 &pos)
{
    return tiles[getTileIdAt(pos)];
}

Tile& Rectangle::getTile(const int tileId)
{
    assert(tileId >= 0 && tileId < getNumTiles());
    return tiles[tileId];
}


Vector3 Rectangle::getTileCenter(int id) const
{
    int ty = id / hNumTiles;
    int tx = id % hNumTiles;

    if (tx < 0) tx = 0;
    if (tx >= hNumTiles) tx = hNumTiles - 1;
    if (ty < 0) ty = 0;
    if (ty >= vNumTiles) ty = vNumTiles - 1;

    //return pos + width_per_tile * (tx+0.5) + height_per_tile * (ty+0.5);
    return add(pos, add( mul(width_per_tile, (tx+0.5)), mul( height_per_tile, (ty+0.5))));
    //return pos;
}
    
Color3 Rectangle::getColor(const Vector3 &pos) const 
{ 
    int tile_id = getTileIdAt(pos);
    /*Vector3 p = getTileCenter(tile_id);
    
    float dist = (pos - p).length();
    //cout << dist << endl;
    float fac = (1-dist/7.0);*/
    return tiles[tile_id].getCombinedColor();//*fac*fac;
    //return Color3(r,g,b); 

}
    
void Rectangle::setTileColor(const int tileId, const Color3 &color) {
    assert (tileId >= 0 && tileId < hNumTiles*vNumTiles);
    tiles[tileId].setLightColor(color);
}

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

void Rectangle::saveAs(const char *filename) const
{
    uint8_t *data = new uint8_t[hNumTiles*vNumTiles*3];
    for (int i = 0; i < hNumTiles*vNumTiles; i++)
    {
            /*col.r = 1 - exp(-col.r);
            col.g = 1 - exp(-col.g);
            col.b = 1 - exp(-col.b);*/

    
        data[i*3+0] = clamp( convert(tiles[i].getCombinedColor().r)*255);
        data[i*3+1] = clamp( convert(tiles[i].getCombinedColor().g)*255);
        data[i*3+2] = clamp( convert(tiles[i].getCombinedColor().b)*255);
    }
    
    write_png_file(filename, hNumTiles, vNumTiles, PNG_COLOR_TYPE_RGB, data);
}



Plane::Plane( const Vector3 &_pos, const Vector3 &_n, const Color3 &_col):  pos(_pos), n(_n), tile(_col) {}

float Plane::intersects( Vector3 ray_src, Vector3 ray_dir, float) {
    float denom = dot(n,ray_dir);
    if (denom == 0)
        return -1;
        
    //return n.dot( pos - ray_src ) / denom;
    return dot(n, sub(pos, ray_src) ) / denom;
}

Vector3 Plane::getNormalAt(const Vector3&) const       { return n;   }
int Plane::getNumTiles() const                 { return 1;   }
int Plane::getTileIdAt(const Vector3 &) const  { return 0;   }
Vector3 Plane::getTileCenter(int) const        { return pos; }

Color3 Plane::getColor(const Vector3 &) const { return tile.getCombinedColor(); };

void Plane::setTileColor(const int, const Color3 &color) {tile.setLightColor(color); }

    
/*
class Sphere: public SceneObject {
public:
    Sphere( const Vector3 &_pos, float _r, const Color3 &_col = Color3(0.5, 0.5, 0.5) ):  pos(_pos), r(_r), col(_col) {}

    float intersects( Vector3 ray_src, Vector3 ray_dir) {
        Vector3 center_line = ray_src - pos;
        float tmp = ray_dir.dot(center_line);
        float s1 = tmp * tmp;
        float s2 = center_line.squaredLength();
        float q = s1 - s2 + r * r;

        if (q < 0)
            return -1.0;
            
        float d1 = - tmp + sqrt(q);
        float d2 = - tmp - sqrt(q);
        
        if (d1 < 0) 
            return d2;

        if (d2 < 0) 
            return d1;
            
        return d1 < d2 ? d1 : d2;
    }

    Vector3 getNormalAt(const Vector3 &p) const{
        return (p - pos).normalized();
    }

    virtual Color3 getColor() const { return col; };
    
private:
    Vector3     pos;
    float      r;
    Color3      col;
};*/

