
#include <vector>
#include <list>
#include "parseLayout.h"

#include <assert.h>
#include <string>
#include "png_helper.h"
#include <iostream>

using namespace std;

string getColorName(uint32_t col)
{
    switch (col)
    {
        case 0xFF000000: return "black";
        case 0xFF808080: return "gray";
        case 0xFFFFFFFF: return "white";
        case 0xFF00FF00: return "green";
        default: return "(unknown)";
    }
   
}

static const Vector3 wallColor = createVector3(0.8, 0.8, 0.8);
static const Vector3 windowColor = createVector3(18, 16, 12);

static const uint32_t BLACK = 0xFF000000;
static const uint32_t WHITE = 0xFFFFFFFF;
//static const uint32_t GRAY =  0xFF808080;
static const uint32_t GREEN = 0xFF00FF00;

static const float HEIGHT = 2.50 * 100;
static const float WINDOW_LOW = 0.90 * 100;
static const float WINDOW_HIGH= 2.20 * 100;
static const float WINDOW_HEIGHT = WINDOW_HIGH - WINDOW_LOW;
static const float TOP_WALL_HEIGHT = HEIGHT - WINDOW_HIGH;


void addWindowedWall(float startX, float startY, float dx, float dy, float scaling, /*ref*/ vector<Rectangle> &wallsRef, vector<Rectangle> &windowsRef)
{
    startX *= scaling;
    startY *= scaling;
    dx *= scaling;
    dy *= scaling;
    
    wallsRef.push_back(createRectangle(  createVector3(startX,startY,0),
                                         createVector3(dx, dy, 0),
                                         createVector3(0, 0, WINDOW_LOW)));
                                         
    windowsRef.push_back(createRectangle(createVector3(startX,startY,WINDOW_LOW),
                                         createVector3(dx, dy, 0),
                                         createVector3(0, 0, WINDOW_HEIGHT)));
                                         
    wallsRef.push_back(createRectangle(  createVector3(startX,startY,WINDOW_HIGH),
                                         createVector3(dx, dy, 0),
                                         createVector3(0, 0, TOP_WALL_HEIGHT)));
}

void addWall(float startX, float startY, float dx, float dy, float scaling, /*ref*/ vector<Rectangle> &segments)
{
    startX *= scaling;
    startY *= scaling;
    dx *= scaling;
    dy *= scaling;

    segments.push_back(createRectangle( createVector3(startX,startY,0),
                                        createVector3(dx,dy,0),
                                        createVector3(0,0,HEIGHT)));
}


void getAABB( const vector<Rectangle> &walls, const vector<Rectangle> &windows, float &min_x, float &max_x, float &min_y, float &max_y)
{
    vector<Rectangle> segments;
    segments.insert(segments.end(), walls.begin(), walls.end());
    segments.insert(segments.end(), windows.begin(), windows.end());
    
    min_x = segments.begin()->pos.s[0];
    max_x = segments.begin()->pos.s[0];
    min_y = segments.begin()->pos.s[1];
    max_y = segments.begin()->pos.s[1];
    for (vector<Rectangle>::const_iterator seg = segments.begin(); seg != segments.end(); seg++)
    {
        max_x = max(max_x, seg->pos.s[0]);
        min_x = min(min_x, seg->pos.s[0]);
        max_y = max(max_y, seg->pos.s[1]);
        min_y = min(min_y, seg->pos.s[1]);
        
        float x = seg->pos.s[0] + seg->width.s[0]; //width may be negative, so pos+width can
        float y = seg->pos.s[1] + seg->width.s[1]; //be smaller or larger than pos alone

        max_x = max(max_x, x);
        min_x = min(min_x, x);
        max_y = max(max_y, y);
        min_y = min(min_y, y);
    }
}

//#error continuehere: add scaling factor, scale height, use real height values
void parseLayout(const char* const filename, const float scaling, vector<Rectangle> &wallsOut, vector<Rectangle> &windowsOut)
{
    wallsOut.clear();
    windowsOut.clear();
    
    int width, height, color_type;
    uint32_t *pixel_buffer;
    read_png_file(filename, &width, &height, &color_type, (uint8_t**)&pixel_buffer );
    //cout << "read image '" << filename << "' of size " << width << "x" << height << " with color_type " << color_type << endl;

    if (color_type == PNG_COLOR_TYPE_RGB)
    {
        uint8_t *src = (uint8_t*)pixel_buffer;
        pixel_buffer = (uint32_t*)malloc( width*height*sizeof(uint32_t));
        
        for (int i = 0; i < width*height; i++)
            pixel_buffer[i] =
                0xFF000000 | src[i*3] | (src[i*3+1] << 8) | (src[i*3+2] << 16);
        
        free(src);
        color_type = PNG_COLOR_TYPE_RGBA;
    }
  
    assert (color_type == PNG_COLOR_TYPE_RGBA);
    
    //segments.push_back(WallSegment( 0, 0, 2, 2));
    
    for (int y = 1; y < height; y++)
    {
        for (int x = 1; x < width;) {
            uint32_t pxAbove = pixel_buffer[(y-1) * width + (x)];
            uint32_t pxHere =  pixel_buffer[(y  ) * width + (x)];
            if (pxAbove == pxHere)
            {
                x++;
                continue;
            }
                
            int startX = x;
            
            while ( x < width && 
                   pxAbove == pixel_buffer[(y-1) * width + (x)] && 
                   pxHere == pixel_buffer[(y) * width + (x)])
                x++;
                
            /*assert(pxAbove != pxHere);
            if (pxAbove != WHITE && pxHere != WHITE)
                continue;*/

            int endX = x;
            
            if      (pxAbove == BLACK && pxHere == WHITE) addWall(startX, y, endX - startX, 0, scaling, wallsOut); //transition from wall to inside area
            else if (pxAbove == WHITE && pxHere == BLACK) addWall(endX,   y, startX - endX, 0, scaling, wallsOut);// transition from inside area to wall
            else if (pxAbove == GREEN && pxHere == WHITE) addWindowedWall(startX, y, endX - startX, 0, scaling, wallsOut, windowsOut); //transition from window to inside area
            else if (pxAbove == WHITE && pxHere == GREEN) addWindowedWall(endX,   y, startX - endX, 0, scaling, wallsOut, windowsOut);
        }
    }
    //cout << "  == End of horizontal scan, beginning vertical scan ==" << endl;

    for (int x = 1; x < width; x++)
    {
        for (int y = 1; y < height; ) {
            uint32_t pxLeft = pixel_buffer[y * width + (x - 1) ];
            uint32_t pxHere = pixel_buffer[y * width + (x    ) ];
            if (pxLeft == pxHere)
            {
                y++;
                continue;
            }
                
            int startY = y;
            
            while (y < height && 
                   pxLeft == pixel_buffer[y * width + (x-1)] && 
                   pxHere == pixel_buffer[y * width + x])
                y++;
                
            int endY = y;
            
            if (pxLeft == BLACK && pxHere == WHITE)      addWall(x, endY,   0, startY - endY, scaling, wallsOut); //transition from wall to inside area
            else if (pxLeft == WHITE && pxHere == BLACK) addWall(x, startY, 0, endY - startY, scaling, wallsOut);// transition from inside area to wall
            else if (pxLeft == GREEN && pxHere == WHITE) addWindowedWall(x, endY,   0, startY - endY, scaling, wallsOut, windowsOut);//transition from window to inside area
            else if (pxLeft == WHITE && pxHere == GREEN) addWindowedWall(x, startY, 0, endY - startY, scaling, wallsOut, windowsOut);
        }
    }

    free( pixel_buffer);

    //find AABB of apartment, as this is a conservative estimate for the floor and ceiling extents
    float min_x, max_x, min_y, max_y;
    getAABB( wallsOut, windowsOut, min_x, max_x, min_y, max_y);
    
    cout << "[INF] Geometry extents are " << (max_x - min_x)/100 << "m x " << (max_y - min_y)/100 << "m" << endl;

    //cout << "floor/ceiling size is: (" << min_x << ", " << min_y << "), " << (max_x - min_x) << "x" << (max_y - min_y) << endl;

    wallsOut.push_back( createRectangle( createVector3(min_x,min_y,HEIGHT), 
                                         createVector3(max_x - min_x, 0, 0), 
                                         createVector3(0, max_y - min_y, 0)));  // ceiling
    wallsOut.push_back( createRectangle( createVector3(min_x,min_y,0), 
                                         createVector3(0, max_y-min_y, 0), 
                                         createVector3(max_x-min_x, 0, 0)));    // floor
    
    //return vector<ExtendedRectangle>(segments.begin(), segments.end());
}

