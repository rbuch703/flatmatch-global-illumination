#include "sceneObject.h"

static const double TILE_SIZE = 10;


Rectangle::Rectangle( const Vector3 &_pos, const Vector3 &_width, const Vector3 &_height):
        pos(_pos), width(_width), height(_height), 
        n(normalized(height.cross(width))),
        width_norm (normalized(width)),
        height_norm (normalized(height)),
        hLength( width.length()),
        vLength(height.length()) {
    /*r = 0.5;
    g = rand() / (double)RAND_MAX;
    b = rand() / (double)RAND_MAX;*/
    
    hNumTiles = round(width.length() / TILE_SIZE);
    if (hNumTiles < 1) hNumTiles = 1;
    
    vNumTiles = round(height.length()/ TILE_SIZE);
    if (vNumTiles < 1) vNumTiles = 1;
    
    tiles = new Tile[vNumTiles * hNumTiles];
    
    width_per_tile = width / hNumTiles;
    height_per_tile= height/ vNumTiles;
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
    g = rand() / (double)RAND_MAX;
    b = rand() / (double)RAND_MAX;*/
    
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
double Rectangle::intersects( Vector3 ray_src, Vector3 ray_dir) {
    if (ray_dir.dot(n) > 0) return -1; //backface culling
    double denom = n.dot(ray_dir);
    if (denom == 0)
        return -1;
        
    double fac = n.dot( pos - ray_src ) / denom;
    if (fac < 0) return fac;    //is behind camera, cannot be hit
    
    Vector3 p = ray_src + ray_dir * fac;
    Vector3 pDir = p - pos;
    
    double dx, dy;
    
    dx = width_norm.dot(pDir);
    dy = height_norm.dot(pDir);
    
    if (dx < 0 || dy < 0) return -1;
    if (dx > hLength || dy > vLength) return -1;
    return fac;
    
    
}

Vector3 Rectangle::normalAt(const Vector3&) {
    return n;
}

int Rectangle::getNumTiles() const
{
    return hNumTiles * vNumTiles;
}

int Rectangle::getTileIdAt(const Vector3 &p) const
{
    Vector3 pDir = p - pos; //vector from rectangle origin (its lower left corner) to current point
    double dx, dy;
    
    dx = width_norm.dot(pDir);
    dy = height_norm.dot(pDir);
    int tx = (dx*hNumTiles) / hLength;
    int ty = (dy*vNumTiles) / vLength;
    if (tx < 0) tx = 0;
    if (tx >= hNumTiles) tx = hNumTiles - 1;
    if (ty < 0) ty = 0;
    if (ty >= vNumTiles) ty = vNumTiles - 1;
    
    assert(ty*hNumTiles + tx <= getNumTiles());
    return ty*hNumTiles + tx;
}

Vector3 Rectangle::getTileCenter(int id) const
{
    int ty = id / hNumTiles;
    int tx = id % hNumTiles;

    if (tx < 0) tx = 0;
    if (tx >= hNumTiles) tx = hNumTiles - 1;
    if (ty < 0) ty = 0;
    if (ty >= vNumTiles) ty = vNumTiles - 1;

    return pos + width_per_tile * (tx+0.5) + height_per_tile * (ty+0.5);
    //return pos;
}
    
Color3 Rectangle::getColor(const Vector3 &pos) const 
{ 
    int tile_id = getTileIdAt(pos);
    /*Vector3 p = getTileCenter(tile_id);
    
    double dist = (pos - p).length();
    //cout << dist << endl;
    double fac = (1-dist/7.0);*/
    return tiles[tile_id].getColor();//*fac*fac;
    //return Color3(r,g,b); 

}
    
void Rectangle::setTileColor(const int tileId, const Color3 &color) {
    assert (tileId >= 0 && tileId < hNumTiles*vNumTiles);
    tiles[tileId].setLightColor(color);
}
    
/*
class Sphere: public SceneObject {
public:
    Sphere( const Vector3 &_pos, double _r, const Color3 &_col = Color3(0.5, 0.5, 0.5) ):  pos(_pos), r(_r), col(_col) {}

    double intersects( Vector3 ray_src, Vector3 ray_dir) {
        Vector3 center_line = ray_src - pos;
        double tmp = ray_dir.dot(center_line);
        double s1 = tmp * tmp;
        double s2 = center_line.squaredLength();
        double q = s1 - s2 + r * r;

        if (q < 0)
            return -1.0;
            
        double d1 = - tmp + sqrt(q);
        double d2 = - tmp - sqrt(q);
        
        if (d1 < 0) 
            return d2;

        if (d2 < 0) 
            return d1;
            
        return d1 < d2 ? d1 : d2;
    }

    Vector3 normalAt(const Vector3 &p) {
        return (p - pos).normalized();
    }

    virtual Color3 getColor() const { return col; };
    
private:
    Vector3     pos;
    double      r;
    Color3      col;
};*/

