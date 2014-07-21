
#include <vector>
#include <list>
#include "parseLayout.h"

#include <assert.h>
#include <string>
#include "png_helper.h"
#include <iostream>

using namespace std;

static const uint8_t INVALIDATED = 0x0;
static const uint8_t WALL = 0x1;
static const uint8_t EMPTY = 0x2;
static const uint8_t OUTSIDE =  0x3;
static const uint8_t DOOR = 0x4;
static const uint8_t WINDOW = 0x5;


static const float HEIGHT = 2.50 * 100;
static const float DOOR_HEIGHT = 2.00 * 100;
static const float WINDOW_LOW = 0.90 * 100;
static const float WINDOW_HIGH= 2.20 * 100;
static const float WINDOW_HEIGHT = WINDOW_HIGH - WINDOW_LOW;
static const float TOP_WALL_HEIGHT = HEIGHT - WINDOW_HIGH;


void addWall( vector<Rectangle> &segments, float startX, float startY, float dx, float dy, float min_z = 0.0f, float max_z = HEIGHT)
{
    segments.push_back(createRectangle( startX,startY,min_z,   dx, dy, 0,       0, 0, max_z - min_z));
}

void addHorizontalRect(vector<Rectangle> &segments, float startX, float startY, float dx, float dy, float z)
{
    segments.push_back(createRectangle( startX,startY,z,       dx, 0, 0,        0, dy, 0));
}

void registerWall( vector<Rectangle> &walls, vector<Rectangle> &windows, 
                   uint32_t col0, uint32_t col1, float x0, float y0, float x1, float y1)
{

    if      (col0 == WALL && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1); //transition from wall to inside area
    else if (col0 == EMPTY && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0);// transition from inside area to wall

    else if (col0 == WALL && col1 == DOOR) addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, DOOR_HEIGHT); //transition from wall to door frame
    else if (col0 == DOOR && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, DOOR_HEIGHT);// 

    else if (col0 == WALL && col1 == WINDOW) addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)
    else if (col0 == WINDOW && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)

    else if (col0 == OUTSIDE  && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1); //transition from entrace to inside area
    else if (col0 == EMPTY && col1 == OUTSIDE ) addWall(walls, x1, y0, x0 - x1, y1 - y0);// transition from entrace to inside area

    else if (col0 == OUTSIDE  && col1 == WINDOW) addWall(windows, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, WINDOW_HIGH);
    else if (col0 == WINDOW && col1 == OUTSIDE)  addWall(windows, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, WINDOW_HIGH);

    else if (col0 == DOOR && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1, DOOR_HEIGHT, HEIGHT); //transition from door frame to inside area
    else if (col0 == EMPTY && col1 == DOOR) addWall(walls, x1, y0, x0 - x1, y1 - y0, DOOR_HEIGHT, HEIGHT);// transition from door frame to inside area

    else if (col0 == WINDOW && col1 == EMPTY) { //transition from window to inside area
        addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, WINDOW_LOW); 
        addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_HIGH, HEIGHT); 
    }
    
    else if (col0 == EMPTY && col1 == WINDOW) {
        addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, WINDOW_LOW);
        addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_HIGH, HEIGHT);
    }


}

void parseLayout(const char* const filename, const float scaling, vector<Rectangle> &wallsOut, vector<Rectangle> &windowsOut)
{
    wallsOut.clear();
    windowsOut.clear();
    
    int width, height, color_type;
    uint32_t *pixel_buffer;
    read_png_file(filename, &width, &height, &color_type, (uint8_t**)&pixel_buffer );

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

    uint8_t *pixels = new uint8_t[width*height];
    for (int i = 0; i < width*height; i++)
    {
        switch (pixel_buffer[i])
        {
            case 0x00000000: pixels[i] = INVALIDATED; break;
            case 0xFF000000: pixels[i] = WALL; break;
            case 0xFFFFFFFF: pixels[i] = EMPTY; break;
            case 0xFF7F7F7F: pixels[i] = OUTSIDE; break;
            case 0xFF00FF00: pixels[i] = WINDOW; break;
            case 0xFFDFDFDF: pixels[i] = DOOR; break;
        }
    }
    free (pixel_buffer);
    
    for (int y = 1; y < height; y++)
    {
        //cout << "scanning row " << y << endl;
        for (int x = 1; x < width;) {
            uint32_t pxAbove = pixels[(y-1) * width + (x)];
            uint32_t pxHere =  pixels[(y  ) * width + (x)];
            if (pxAbove == pxHere)
            {
                x++;
                continue;
            }
                
            float startX = x;
            
            while ( x < width && 
                   pxAbove == pixels[(y-1) * width + (x)] && 
                   pxHere == pixels[(y) * width + (x)])
                x++;
                
            float endX = x;
            
            registerWall(wallsOut, windowsOut, pxAbove, pxHere, startX*scaling, y*scaling, endX*scaling, y*scaling);
        }
    }
    //cout << "  == End of horizontal scan, beginning vertical scan ==" << endl;

    for (int x = 1; x < width; x++)
    {
        for (int y = 1; y < height; ) {
            uint32_t pxLeft = pixels[y * width + (x - 1) ];
            uint32_t pxHere = pixels[y * width + (x    ) ];
            if (pxLeft == pxHere)
            {
                y++;
                continue;
            }
                
            float startY = y;
            
            while (y < height && 
                   pxLeft == pixels[y * width + (x-1)] && 
                   pxHere == pixels[y * width + x])
                y++;
                
            float endY = y;
            
            registerWall(wallsOut, windowsOut, pxLeft, pxHere, x*scaling, startY*scaling, x*scaling, endY*scaling);
        }
    }

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
    {
        int xStart = x;
        uint32_t color = pixels[y * width + x];
        if (color == INVALIDATED)
            continue;
        
        while (x+1 < width && pixels[y*width + (x+1)] == color) 
            x++;
            
        int xEnd = x;
        
        int yEnd;
        for (yEnd = y + 1; yEnd < height; yEnd++)
        {
            bool rowWithIdenticalColor = true;
            for (int xi = xStart; xi <= xEnd && rowWithIdenticalColor; xi++)
                rowWithIdenticalColor &= (pixels[yEnd * width + xi] == color);
            
            if (! rowWithIdenticalColor)
                break;
        }
        
        yEnd--;

        for (int yi = y; yi <= yEnd; yi++)
            for (int xi = xStart; xi <= xEnd; xi++)
                pixels[yi * width + xi] = INVALIDATED;

        yEnd +=1;   //cover the area to the end of the pixel, not just to the start
        xEnd +=1;        
        switch (color)
        {
            case WINDOW: //window --> create upper and lower window frame
                addHorizontalRect(wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), WINDOW_LOW); 
                addHorizontalRect(wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), WINDOW_HIGH); 
                break;

            case EMPTY: //empty floor --> create floor and ceiling
                addHorizontalRect(wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0); 
                addHorizontalRect(wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), HEIGHT); 
                break;
                
            case DOOR: 
                addHorizontalRect(wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0); 
                addHorizontalRect(wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), DOOR_HEIGHT); 
            break;

        
        }
    }
    

    delete [] pixels;
}

