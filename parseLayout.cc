
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
static const uint32_t GRAY =  0xFF7F7F7F;
static const uint32_t LGRAY = 0xFFDFDFDF;
static const uint32_t GREEN = 0xFF00FF00;
static const uint32_t INVALIDATED = 0x00000000;


static const float HEIGHT = 2.50 * 100;
static const float DOOR_HEIGHT = 2.00 * 100;
static const float WINDOW_LOW = 0.90 * 100;
static const float WINDOW_HIGH= 2.20 * 100;
static const float WINDOW_HEIGHT = WINDOW_HIGH - WINDOW_LOW;
static const float TOP_WALL_HEIGHT = HEIGHT - WINDOW_HIGH;


void addWindowedWall(float startX, float startY, float dx, float dy, /*ref*/ vector<Rectangle> &wallsRef, vector<Rectangle> *windowsRef)
{
    wallsRef.push_back(createRectangle(  createVector3(startX,startY,0),
                                         createVector3(dx, dy, 0),
                                         createVector3(0, 0, WINDOW_LOW)));
   
    if (windowsRef)                      
        windowsRef->push_back(createRectangle(createVector3(startX,startY,WINDOW_LOW),
                                         createVector3(dx, dy, 0),
                                         createVector3(0, 0, WINDOW_HEIGHT)));
                                         
    wallsRef.push_back(createRectangle(  createVector3(startX,startY,WINDOW_HIGH),
                                         createVector3(dx, dy, 0),
                                         createVector3(0, 0, TOP_WALL_HEIGHT)));
}

void addWall(/*ref*/ vector<Rectangle> &segments, float startX, float startY, float dx, float dy, float min_z = 0.0f, float max_z = HEIGHT)
{

    segments.push_back(createRectangle( createVector3(startX,startY,min_z),
                                        createVector3(dx,dy,0),
                                        createVector3(0,0,max_z - min_z)));
}

void addHorizontalRect(vector<Rectangle> &segments, float startX, float startY, float dx, float dy, float z)
{
    segments.push_back(createRectangle( createVector3(startX,startY,z),
                                        createVector3(dx,0,0),
                                        createVector3(0,dy,0)));


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

void parseLayout(const char* const filename, const float scaling, vector<Rectangle> &wallsOut, vector<Rectangle> &windowsOut)
{
    wallsOut.clear();
    windowsOut.clear();
    
    int width, height, color_type;
    uint32_t *pixel_buffer;
    //uint32_t *pixel_debug;
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

    //pixel_debug = (uint32_t*) malloc( width*height*sizeof(uint32_t));
    
    //segments.push_back(WallSegment( 0, 0, 2, 2));
    
    for (int y = 1; y < height; y++)
    {
        //cout << "scanning row " << y << endl;
        for (int x = 1; x < width;) {
            uint32_t pxAbove = pixel_buffer[(y-1) * width + (x)];
            uint32_t pxHere =  pixel_buffer[(y  ) * width + (x)];
            if (pxAbove == pxHere)
            {
                x++;
                continue;
            }
                
            float startX = x;
            
            while ( x < width && 
                   pxAbove == pixel_buffer[(y-1) * width + (x)] && 
                   pxHere == pixel_buffer[(y) * width + (x)])
                x++;
                
            float endX = x;
            
            startX *= scaling;
            endX *= scaling;
            float sy  = y * scaling;
            
            
            if      (pxAbove == BLACK && pxHere == WHITE) addWall(wallsOut, startX, sy, endX - startX, 0); //transition from wall to inside area
            else if (pxAbove == WHITE && pxHere == BLACK) addWall(wallsOut, endX,   sy, startX - endX, 0);// transition from inside area to wall

            if      (pxAbove == BLACK && pxHere == LGRAY) addWall(wallsOut, startX, sy, endX - startX, 0, 0, DOOR_HEIGHT); //transition from wall to door frame
            else if (pxAbove == LGRAY && pxHere == BLACK) addWall(wallsOut, endX,   sy, startX - endX, 0, 0, DOOR_HEIGHT);// 


            else if (pxAbove == GRAY  && pxHere == WHITE) addWall(wallsOut, startX, sy, endX - startX, 0); //transition from entrace to inside area
            else if (pxAbove == WHITE && pxHere == GRAY ) addWall(wallsOut, endX,   sy, startX - endX, 0);// transition from entrace to inside area

            if      (pxAbove == LGRAY && pxHere == WHITE) addWall(wallsOut, startX, sy, endX - startX, 0, DOOR_HEIGHT, HEIGHT); //transition from door frame to inside area
            else if (pxAbove == WHITE && pxHere == LGRAY) addWall(wallsOut, endX,   sy, startX - endX, 0, DOOR_HEIGHT, HEIGHT);// transition from door frame to inside area


            else if (pxAbove == BLACK && pxHere == GREEN) addWall(wallsOut, startX, sy, endX - startX, 0, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)
            else if (pxAbove == GREEN && pxHere == BLACK) addWall(wallsOut, endX, sy, startX - endX,   0, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)


            else if (pxAbove == GREEN && pxHere == WHITE) addWindowedWall(startX, sy, endX - startX, 0, wallsOut, NULL); //transition from window to inside area
            else if (pxAbove == WHITE && pxHere == GREEN) addWindowedWall(endX,   sy, startX - endX, 0, wallsOut, NULL);

            else if (pxAbove == GRAY  && pxHere == GREEN) addWall(windowsOut, startX, sy, endX - startX, 0, WINDOW_LOW, WINDOW_HIGH);
            else if (pxAbove == GREEN && pxHere == GRAY)  addWall(windowsOut, endX, sy, startX - endX, 0, WINDOW_LOW, WINDOW_HIGH);

            //else
            //    cout << "unknown transition " << hex << pxAbove << "/" << pxHere << dec << endl;


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
                
            float startY = y;
            
            while (y < height && 
                   pxLeft == pixel_buffer[y * width + (x-1)] && 
                   pxHere == pixel_buffer[y * width + x])
                y++;
                
            float endY = y;
            
            startY *= scaling;
            endY *= scaling;
            float sx  = x * scaling;
            
            
            if      (pxLeft == BLACK && pxHere == WHITE) addWall(wallsOut, sx, endY,   0, startY - endY); //transition from wall to inside area
            else if (pxLeft == WHITE && pxHere == BLACK) addWall(wallsOut, sx, startY, 0, endY - startY);// transition from inside area to wall

            else if (pxLeft == BLACK && pxHere == LGRAY) addWall(wallsOut, sx, endY,   0, startY - endY, 0, DOOR_HEIGHT); //transition from wall to door frame
            else if (pxLeft == LGRAY && pxHere == BLACK) addWall(wallsOut, sx, startY, 0, endY - startY, 0, DOOR_HEIGHT); // 

            if      (pxLeft == LGRAY && pxHere == WHITE) addWall(wallsOut, sx, endY,   0, startY - endY, DOOR_HEIGHT, HEIGHT); //transition from wall to inside area
            else if (pxLeft == WHITE && pxHere == LGRAY) addWall(wallsOut, sx, startY, 0, endY - startY, DOOR_HEIGHT, HEIGHT);// transition from inside area to wall


            else if (pxLeft == GRAY  && pxHere == WHITE) addWall(wallsOut, sx, endY,   0, startY - endY); //transition from wall to inside area
            else if (pxLeft == WHITE && pxHere == GRAY ) addWall(wallsOut, sx, startY, 0, endY - startY);// transition from inside area to wall

            else if (pxLeft == BLACK && pxHere == GREEN) addWall(wallsOut, sx, endY,   0, startY - endY, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)
            else if (pxLeft == GREEN && pxHere == BLACK) addWall(wallsOut, sx, startY, 0, endY - startY, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)

            else if (pxLeft == GREEN && pxHere == WHITE) addWindowedWall(sx, endY,     0, startY - endY, wallsOut, NULL);//transition from window to inside area
            else if (pxLeft == WHITE && pxHere == GREEN) addWindowedWall(sx, startY,   0, endY - startY, wallsOut, NULL);

            else if (pxLeft == GRAY  && pxHere == GREEN) addWall(windowsOut, sx, endY,   0, startY - endY, WINDOW_LOW, WINDOW_HIGH);
            else if (pxLeft == GREEN && pxHere == GRAY)  addWall(windowsOut, sx, startY, 0, endY - startY, WINDOW_LOW, WINDOW_HIGH);

        }
    }

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
    {
        int xStart = x;
        uint32_t color = pixel_buffer[y * width + x];
        if (color == INVALIDATED)
            continue;
        
        while (x+1 < width && pixel_buffer[y*width + (x+1)] == color) 
            x++;
            
        int xEnd = x;
        
        int yEnd;
        for (yEnd = y + 1; yEnd < height; yEnd++)
        {
            bool rowWithIdenticalColor = true;
            for (int xi = xStart; xi <= xEnd && rowWithIdenticalColor; xi++)
                rowWithIdenticalColor &= (pixel_buffer[yEnd * width + xi] == color);
            
            if (! rowWithIdenticalColor)
                break;
        }
        
        yEnd--;

        //uint32_t randomColor =  (uint32_t)(rand() / (double)RAND_MAX * (1 << 24)) | 0xFF000000;
        
        for (int yi = y; yi <= yEnd; yi++)
            for (int xi = xStart; xi <= xEnd; xi++)
            {
                pixel_buffer[yi * width + xi] = INVALIDATED;
                //pixel_debug[yi * width + xi] = randomColor;
            }

        yEnd +=1;   //cover the area to the end of the pixel, not just to the start
        xEnd +=1;        
        switch (color)
        {
            case GREEN: //window --> create upper and lower window frame
                addHorizontalRect(wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), WINDOW_LOW); 
                addHorizontalRect(wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), WINDOW_HIGH); 
                break;

            case WHITE: //empty floor --> create floor and ceiling
                addHorizontalRect(wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0); 
                addHorizontalRect(wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), HEIGHT); 
                break;
                
            case LGRAY: 
                addHorizontalRect(wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0); 
                addHorizontalRect(wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), DOOR_HEIGHT); 
            break;

        
        }
        
        //cout << "found rectangle (" << xStart << ", " << y << ") - (" << xEnd << ", " << yEnd << ")" << " with color " << hex << color << dec << endl;
        
    }
    
    //write_png_file("rects.png", width, height, PNG_COLOR_TYPE_RGBA, (uint8_t*)pixel_debug);
    //exit(0);

    free( pixel_buffer);
    //free(pixel_debug);

    //find AABB of apartment, as this is a conservative estimate for the floor and ceiling extents
    float min_x, max_x, min_y, max_y;
    getAABB( wallsOut, windowsOut, min_x, max_x, min_y, max_y);
    
    cout << "[INF] Geometry extents are " << (max_x - min_x)/100 << "m x " << (max_y - min_y)/100 << "m" << endl;

    //cout << "floor/ceiling size is: (" << min_x << ", " << min_y << "), " << (max_x - min_x) << "x" << (max_y - min_y) << endl;
    /*
    wallsOut.push_back( createRectangle( createVector3(min_x,min_y,HEIGHT), 
                                         createVector3(max_x - min_x, 0, 0), 
                                         createVector3(0, max_y - min_y, 0)));  // ceiling
    wallsOut.push_back( createRectangle( createVector3(min_x,min_y,0), 
                                         createVector3(0, max_y-min_y, 0), 
                                         createVector3(max_x-min_x, 0, 0)));    // floor
    */
    //cout << "registered " << windowsOut.size() << " windows" << endl;
    //return vector<ExtendedRectangle>(segments.begin(), segments.end());
}

