
#include <vector>
#include <list>
#include "parseLayout.h"

#include <assert.h>
#include <string>
#include <string.h> //for memcpy
#include <math.h> //for sqrt()
#include "png_helper.h"
#include <iostream>

using namespace std;

static const uint8_t INVALIDATED = 0x0;
static const uint8_t WALL = 0x1;
static const uint8_t EMPTY = 0x2;
static const uint8_t OUTSIDE =  0x3;
static const uint8_t DOOR = 0x4;
static const uint8_t WINDOW = 0x5;


static const float HEIGHT      = 2.60;
static const float DOOR_HEIGHT = 2.00;
static const float WINDOW_LOW  = 0.85;
static const float WINDOW_HIGH = 2.30;
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

uint32_t getData(int x, int y, uint32_t *pixelBuffer, int width, int height)
{
    x = max( min(x, width-1), 0);
    y = max( min(y, height-1), 0);
    return pixelBuffer[y*width+x];
}

bool setData(int x, int y, uint32_t val, uint32_t *pixelBuffer, int width, int height)
{
    if ( x < 0) return false;
    if ( x >= width) return false;
    if ( y < 0) return false;
    if ( y >= height) return false;
    
    pixelBuffer[y*width+x] = val;
    return true;
}



unsigned int distanceTransform(uint32_t *data, int width, int height)
{

    vector<pair<int, int>> openList;
    unsigned int distance = 1;
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
    {
        if (getData(x,y, data, width, height) == distance)
            openList.push_back(pair<int, int>(x,y));
    }

    while (openList.size())
    {
        //cout << "open list for distance " << distance << " contains " << openList.size() << " entries" << endl;

        vector<pair<int, int>> newOpenList;
        for (unsigned int i = 0; i < openList.size(); i++)
        {
            //int x = openList[i].first;
            //int y = openList[i].second;
            
            for (int x = max(openList[i].first-1, 0); x <= min(openList[i].first + 1, width - 1); x++)
                for (int y = max(openList[i].second - 1, 0); y <= min(openList[i].second + 1, height-1); y++)
                {
                    if (x == openList[i].first && y == openList[i].second)
                        continue;
                        
                    if (getData(x, y, data, width, height) == 0)
                    {
                        setData(x, y, distance+1, data, width, height);
                        newOpenList.push_back(pair<int, int>(x, y));
                    }
                }
        }
        
        
        openList = newOpenList;
        distance += 1;
    }

    /*for (int i = 0; i < width*height; i++)
        data[i] = data[i] * 4 | 0xFF000000;

    write_png_file("distance.png", width, height, PNG_COLOR_TYPE_RGBA, (uint8_t*)data);*/

    
    //the while loop loops until the open list for a distance is completely empty,
    //i.e. until a distance is reached for which not a single pixel exists.
    //so the actual maximum distance returned here is one less than the distance
    //for which the loop terminated
    return distance - 1;

}


pair<int, int> getCentralPosition(uint32_t *pixelBuffer, int width, int height)
{
    uint32_t *tmpData = new uint32_t[width*height];
    memcpy(tmpData, pixelBuffer, width*height*sizeof(uint32_t));
    
    for (int i = 0; i < width*height; i++)
    {
        switch (tmpData[i])
        {
            case 0xFFFFFFFF: 
            case 0xFF00FF00: 
            case 0xFFDFDFDF: 
                tmpData[i] = 0;
                break;
                
            default: 
                tmpData[i] = 1; //walls, outside
                break;
        }
    }

    unsigned int maxDistance = distanceTransform(tmpData, width, height);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            if (tmpData[y*width+x] == maxDistance-1)
            {
                delete [] tmpData;
                return pair<int, int>(x, y);
            }
    
    delete [] tmpData;
    assert(false);
    return pair<int, int>(-1, -1);
}

void floodFill(int x, int y, uint32_t *pixelBuffer, int width, int height, uint32_t value, uint32_t background)
{
    if (getData(x, y, pixelBuffer, width, height) != background)
        return;
        
    setData(x, y, value, pixelBuffer, width, height);
    
    floodFill( x-1, y-1, pixelBuffer, width, height, value, background);
    floodFill( x-1, y  , pixelBuffer, width, height, value, background);
    floodFill( x-1, y+1, pixelBuffer, width, height, value, background);

    floodFill( x  , y-1, pixelBuffer, width, height, value, background);
//  floodFill( x  , y  , pixelBuffer, width, height, value, background);
    floodFill( x  , y+1, pixelBuffer, width, height, value, background);

    floodFill( x+1, y-1, pixelBuffer, width, height, value, background);
    floodFill( x+1, y  , pixelBuffer, width, height, value, background);
    floodFill( x+1, y+1, pixelBuffer, width, height, value, background);
}


int traverseRoom(int x, int y, uint32_t *pixelData, int width, int height, unsigned int &maxDist, pair<int, int> &maxPos)
{
    uint32_t valHere = getData(x, y, pixelData, width, height);
    //cout << "at position " << x << ", " << y << " with value " << valHere << endl;
    assert( valHere > 1);

    setData(x, y, 0, pixelData, width, height);
    int numPixels = 1;
    
    if ( valHere > maxDist)
    {
        maxDist = valHere;
        maxPos = pair<int, int>(x, y);
    }
    
    if ( x > 0       && getData(x-1, y, pixelData, width, height) > 1) numPixels += traverseRoom(x-1, y, pixelData, width, height, maxDist, maxPos);
    if ( x+1 < width && getData(x+1, y, pixelData, width, height) > 1) numPixels += traverseRoom(x+1, y, pixelData, width, height, maxDist, maxPos);
    
    if ( y > 0        && getData(x, y-1, pixelData, width, height) > 1) numPixels += traverseRoom(x, y-1, pixelData, width, height, maxDist, maxPos);
    if ( y+1 < height && getData(x, y+1, pixelData, width, height) > 1) numPixels += traverseRoom(x, y+1, pixelData, width, height, maxDist, maxPos);
    
    return numPixels;
}

void createWindowInRoom(int x, int y, uint32_t *pixelData, int width, int height, float scaling, vector<Rectangle> &windowsOut)
{
    assert( getData(x, y, pixelData, width, height) > 1);
    
    unsigned int maxDist = 1;
    pair<int, int> maxPos = pair<int, int>(x, y);
    int numPixels = traverseRoom(x, y, pixelData, width, height, maxDist, maxPos);    

    cout << "found room with center at (" << maxPos.first << ", " << maxPos.second << ") with distance " << maxDist << " and area " << numPixels << endl;
    float edgeHalfLength = sqrt(numPixels) / 20;
    //maxDist is the maximum distance from maxPos for which it is guaranteed that there is no wall in any direction
    // (might be one-off though)
    
    if (edgeHalfLength > maxDist)
        edgeHalfLength = maxDist;
    
    //cout << "scaling: " << scaling << ", x=" << maxPos.first << ", y=" << maxPos.second << endl;
    edgeHalfLength *=scaling;
    float px = maxPos.first  * scaling;
    float py = maxPos.second * scaling;
    
    //cout << "after scaling: x=" << px << ", y=" << py << endl;
    //cout << "edgeHalfLength is " << edgeHalfLength << endl;
    
    windowsOut.push_back(createRectangle( px - edgeHalfLength, py - edgeHalfLength, HEIGHT-0.001,
                                          2*edgeHalfLength, 0, 0,
                                          0, 2*edgeHalfLength, 0));
/*
    for (int i = 0; i < width*height; i++)
        pixelData[i] |= 0xFF000000;

    setData(maxPos.first, maxPos.second, 0xFF00FF00, pixelData, width, height);
    write_png_file("lamp.png", width, height, PNG_COLOR_TYPE_RGBA, (uint8_t*)pixelData);
    exit(0);*/

}


/* This method finds rooms (including the corridor) that have no windows (and thus will appear too dark)
   and adds ceiling lights to them. This is done in multiple steps:
   1. all inside areas adjacent to windows are flood-filled to exclude them
   2. for the remaining inside areas, a distance transform is performed
   3. for all areas with a distance transform > 1 (i.e. the remaining rooms),
     3a. the center point is found
     3b. the room size is calculated (pixel counting)
     3c. a light source is created at the center point with a size proportional to the room size;
 */
void createLights(const uint32_t *pixelBuffer, int width, int height, float scaling, vector<Rectangle> &windowsOut)
{
    uint32_t *tmpData = new uint32_t[width*height];
    memcpy(tmpData, pixelBuffer, width*height*sizeof(uint32_t));
    
    
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
    {
        if (getData(x, y, tmpData, width, height) == 0xFF00FF00) //window
        {
            if (getData(x-1, y, tmpData, width, height) == 0xFFFFFFFF) floodFill(x-1, y, tmpData, width, height, 0xFF00FF00, 0xFFFFFFFF);
            if (getData(x+1, y, tmpData, width, height) == 0xFFFFFFFF) floodFill(x+1, y, tmpData, width, height, 0xFF00FF00, 0xFFFFFFFF);
            if (getData(x, y-1, tmpData, width, height) == 0xFFFFFFFF) floodFill(x, y-1, tmpData, width, height, 0xFF00FF00, 0xFFFFFFFF);
            if (getData(x, y+1, tmpData, width, height) == 0xFFFFFFFF) floodFill(x, y+1, tmpData, width, height, 0xFF00FF00, 0xFFFFFFFF);
        }
    }

    /*write_png_file( "distance.png", width, height, PNG_COLOR_TYPE_RGBA, (uint8_t*)tmpData);*/
    for (int i = 0; i < width*height; i++)
        tmpData[i] = (tmpData[i] == 0xFFFFFFFF) ? 0 : 1;


    distanceTransform(tmpData, width, height);
    
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
        {
            if (getData(x, y, tmpData, width, height) > 1)
                createWindowInRoom(x, y, tmpData, width, height, scaling, windowsOut);
        }

    /*for (int i = 0; i < width*height; i++)
        tmpData[i] = tmpData[i] * 3 | 0xFF000000;

    write_png_file( "distance.png", width, height, PNG_COLOR_TYPE_RGBA, (uint8_t*)tmpData);
    exit(0);    */
    
    delete [] tmpData;
    //assert(false);
    //return pair<int, int>(-1, -1);
}


void parseLayout(const char* const filename, const float scaling, vector<Rectangle> &wallsOut, vector<Rectangle> &windowsOut, vector<Rectangle> &lightsOut, pair<float, float> &startingPositionOut)
{
    wallsOut.clear();
    windowsOut.clear();
    
    int width, height, color_type;
    uint32_t *pixelBuffer;
    read_png_file(filename, &width, &height, &color_type, (uint8_t**)&pixelBuffer );

    if (color_type == PNG_COLOR_TYPE_RGB)
    {
        uint8_t *src = (uint8_t*)pixelBuffer;
        pixelBuffer = (uint32_t*)malloc( width*height*sizeof(uint32_t));
        
        for (int i = 0; i < width*height; i++)
            pixelBuffer[i] =
                0xFF000000 | src[i*3] | (src[i*3+1] << 8) | (src[i*3+2] << 16);
        
        free(src);
        color_type = PNG_COLOR_TYPE_RGBA;
    }

    assert (color_type == PNG_COLOR_TYPE_RGBA);

    uint8_t *pixels = new uint8_t[width*height];
    for (int i = 0; i < width*height; i++)
    {
        switch (pixelBuffer[i])
        {
            case 0x00000000: pixels[i] = INVALIDATED; break;
            case 0xFF000000: pixels[i] = WALL; break;
            case 0xFFFFFFFF: pixels[i] = EMPTY; break;
            case 0xFF7F7F7F: pixels[i] = OUTSIDE; break;
            case 0xFF00FF00: pixels[i] = WINDOW; break;
            case 0xFFDFDFDF: pixels[i] = DOOR; break;
        }
    }

    pair<int, int> centralPos = getCentralPosition(pixelBuffer, width, height);
    startingPositionOut.first  = centralPos.first * scaling;
    startingPositionOut.second = centralPos.second * scaling;
    
    createLights(pixelBuffer, width, height, scaling, lightsOut);
    
    
    free (pixelBuffer);
    
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

