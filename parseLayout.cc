
#include <assert.h>
#include <string.h> //for memcpy
#include <math.h> //for sqrt()

#include <iostream>
#include <string>
#include <vector>
//#include <list>

#include "parseLayout.h"
#include "png_helper.h"
#include "image.h"

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

void registerWall( vector<Rectangle> &walls, vector<Rectangle> &windows, vector<Rectangle> &box,
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

    else if (col0 == DOOR && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1, DOOR_HEIGHT, HEIGHT); //transition from door frame to inside area
    else if (col0 == EMPTY && col1 == DOOR) addWall(walls, x1, y0, x0 - x1, y1 - y0, DOOR_HEIGHT, HEIGHT);// transition from door frame to inside area

    else if (col0 == WALL    && col1 == OUTSIDE) addWall(box, x0, y1, x1 - x0, y0 - y1, -0.2, HEIGHT + 0.2); 
    else if (col0 == OUTSIDE && col1 == WALL   ) addWall(box, x1, y0, x0 - x1, y1 - y0, -0.2, HEIGHT + 0.2); 


    else if (col0 == WINDOW && col1 == EMPTY) { //transition from window to inside area
        addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, WINDOW_LOW); 
        addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_HIGH, HEIGHT); 
    }
    
    else if (col0 == EMPTY && col1 == WINDOW) {
        addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, WINDOW_LOW);
        addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_HIGH, HEIGHT);
    }

    else if (col0 == OUTSIDE  && col1 == WINDOW) 
    {
        addWall(box,     x1, y0, x0 - x1, y1 - y0, -0.2, WINDOW_LOW);
        addWall(box,     x1, y0, x0 - x1, y1 - y0, WINDOW_HIGH, HEIGHT + 0.2);
        addWall(windows, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, WINDOW_HIGH);
    }
    else if (col0 == WINDOW && col1 == OUTSIDE)  
    {
        addWall(box,     x0, y1, x1 - x0, y0 - y1, -0.2, WINDOW_LOW);
        addWall(box,     x0, y1, x1 - x0, y0 - y1, WINDOW_HIGH, HEIGHT + 0.2);
        addWall(windows, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, WINDOW_HIGH);
    }


}


pair<int, int> getCentralPosition(uint32_t *pixelBuffer, int width, int height)
{
    Image tmpData(width, height, pixelBuffer);

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)    
    {
        switch (tmpData.get(x,y))
        {
            case 0xFFFFFFFF: 
            case 0xFF00FF00: 
            case 0xFFDFDFDF: 
                tmpData.set(x,y,0);
                break;
                
            default: 
                tmpData.set(x,y, 1); //walls, outside
                break;
        }
    }

    unsigned int maxDistance = tmpData.distanceTransform();
    
    
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            if (tmpData.get(x,y) == maxDistance-1)
            {
                return pair<int, int>(x, y);
            }
    
    assert(false);
    return pair<int, int>(-1, -1);
}

int traverseRoom(Image &img, Image &visited, int x, int y, unsigned int &maxDist, vector<pair<int, int>> &skeletalPoints)
{
    std::set< pair<int, int> > candidates;
    std::set< pair<int, int> > visitedPoints;
    
    candidates.insert(pair<int, int>(x,y));

    int numPixels = 0;

    while (! candidates.empty())
    {
        pair<int,int> pos = * (candidates.begin());
        int x = pos.first;
        int y = pos.second;
        visitedPoints.insert(pos);
        candidates.erase(pos);

        if (x < 0 || x >= img.getWidth() ) return 0;
        if (y < 0 || y >= img.getHeight() ) return 0;
        assert( img.getWidth () == visited.getWidth());
        assert( img.getHeight() == visited.getHeight());

        if (img.get(x, y) == 0) continue; //stepped on a wall
        if (visited.get(x,y))   continue; //already been here
        visited.set(x, y, 2);
        numPixels += 1;

        if ( img.get(x,y) >= img.get(x+1, y) && img.get(x,y) >= img.get(x-1, y) &&
             img.get(x,y) >= img.get(x, y+1) && img.get(x,y) >= img.get(x, y-1))
        {
                skeletalPoints.push_back( pair<int, int>(x, y));
                visited.set(x, y, 3);
        }

        
        if ( img.get(x, y) > maxDist)
            maxDist = img.get(x, y);
        
        if (visitedPoints.count(pair<int,int>(x-1, y  )) == 0) candidates.insert( pair<int, int>(x-1,y  ));
        if (visitedPoints.count(pair<int,int>(x+1, y  )) == 0) candidates.insert( pair<int, int>(x+1,y  ));
        if (visitedPoints.count(pair<int,int>(x  , y-1)) == 0) candidates.insert( pair<int, int>(x  ,y-1));
        if (visitedPoints.count(pair<int,int>(x  , y+1)) == 0) candidates.insert( pair<int, int>(x  ,y+1));
    }
    
    return numPixels;
}

int sqr(int a) { return a*a;}

void createLightSourceInRoom(Image &img, Image &visited, int roomX, int roomY, float scaling, vector<Rectangle> &lightsOut)
{
    assert( img.get(roomX, roomY) > 1);
    
    unsigned int maxDist = 1;

    vector<pair<int, int>> skeletalPoints;
    int numPixels = traverseRoom(img, visited, roomX, roomY, maxDist, skeletalPoints);
    assert(skeletalPoints.size() > 0);

    int min_x = skeletalPoints[0].first;
    int max_x = skeletalPoints[0].first;
    int min_y = skeletalPoints[0].second;
    int max_y = skeletalPoints[0].second;
    
    for (unsigned int i = 0; i < skeletalPoints.size(); i++)
    {
        int x = skeletalPoints[i].first;
        int y = skeletalPoints[i].second;
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        
        if ( img.get(x,y)  >= 0.9*maxDist)
            visited.set(x,y,4);
    }
    
    int mid_x = (min_x + max_x) / 2;
    int mid_y = (min_y + max_y) / 2;
    //cout << "Mid point is (" << mid_x << ", " << mid_y << ")" << endl;
    
    pair<int, int> bestCenter = skeletalPoints[0];
    int bestDist = ( sqr( bestCenter.first - mid_x) + sqr( bestCenter.second - mid_y));
    for (unsigned int i = 0; i < skeletalPoints.size(); i++)
    {
        int x = skeletalPoints[i].first;
        int y = skeletalPoints[i].second;
        int dist = ( sqr( x - mid_x) + sqr( y - mid_y));
        if (dist < bestDist)
        {
            bestDist = dist;
            bestCenter = skeletalPoints[i];
        }
    }    
    visited.set( bestCenter.first, bestCenter.second, 5);
    
    cout << "found room with center at (" << bestCenter.first << ", " << bestCenter.second << ") with distance " << maxDist << " and area " << numPixels << endl;
    float edgeHalfLength = sqrt(numPixels) / 7;

    //maxDist is the maximum distance from maxPos for which it is guaranteed that there is no wall in any direction
    if (edgeHalfLength > maxDist-1)
        edgeHalfLength = maxDist-1;
    
    edgeHalfLength *=scaling;
    float px = bestCenter.first  * scaling;
    float py = bestCenter.second * scaling;
    
    lightsOut.push_back(createRectangle( px - edgeHalfLength, py - edgeHalfLength, HEIGHT-0.001,
                                          2*edgeHalfLength, 0, 0,
                                          0, 2*edgeHalfLength, 0));

}


/* This method finds rooms (including the corridor) that have no windows (and thus will appear too dark)
   and adds ceiling lights to them. This is done in multiple steps:
   1. all inside areas adjacent to windows are flood-filled to exclude them
   2. for the remaining inside areas, a distance transform is performed
   3. for all areas with a distance transform > 1 (i.e. the remaining rooms),
     3a. all skeletal pixels are found (pixels for which no neighboring pixel has a higher distance value
     3b. all skeletal pixels are considered whose distance value (cf. 3.) is within 90% of the maximum for this room
     3c. the skeletal pixel closest to the room AABB center is selected as the center point
     3d. the room size is calculated (pixel counting)
     3e. a light source is created at the center point with a size proportional to the room size;
 */
 
 /*Note: the Image is passed by value on purpose to force the compiler to create an Image copy for this destructive operation*/
void createLights(Image img, float scaling, vector<Rectangle> &lightsOut)
{

    //Step 1: Flood-fill rooms adjacent to windows with windo color, to mark them as not requiring additional lighting    
    for (int y = 0; y < img.getHeight(); y++)
        for (int x = 0; x < img.getWidth(); x++)
    {
        if (img.get(x, y) == 0xFF00FF00) //window
        {
            if (img.get(x-1, y  ) == 0xFFFFFFFF) img.floodFill(x-1, y,   0xFF00FF00, 0xFFFFFFFF);
            if (img.get(x+1, y  ) == 0xFFFFFFFF) img.floodFill(x+1, y,   0xFF00FF00, 0xFFFFFFFF);
            if (img.get(x,   y-1) == 0xFFFFFFFF) img.floodFill(x,   y-1, 0xFF00FF00, 0xFFFFFFFF);
            if (img.get(x,   y+1) == 0xFFFFFFFF) img.floodFill(x,   y+1, 0xFF00FF00, 0xFFFFFFFF);
        }
    }
    img.saveAs("filled.png");

    //prepare map for distance transform: only empty space inside the apartment is considered ("0")
    for (int y = 0; y < img.getHeight(); y++)
        for (int x = 0; x < img.getWidth(); x++)
            img.set(x, y, (img.get(x,y) == 0xFFFFFFFF) ? 0 : 1);

    //Step 2, the actual distance transform
    img.distanceTransform();

    // prepare map of pixels that have already been considered for placing lights (initially only those not used in the distance transform
    Image visited(img.getWidth(), img.getHeight());
    for (int y = 0; y < img.getHeight(); y++)
        for (int x = 0; x < img.getWidth(); x++)
            if (img.get(x,y) == 1) //WALL
                visited.set(x,y,1);

    // The actual steps 3a-e are performed by createLightSourceInRoom(), which also updates the 'visited' map
    for (int y = 0; y < img.getHeight(); y++)
        for (int x = 0; x < img.getWidth(); x++)
        {
            if (img.get(x, y) > 1 && !visited.get(x, y))
                createLightSourceInRoom(img, visited, x, y, scaling, lightsOut);
        }

}


void parseLayout(const char* const filename, const float scaling, int& width, int& height, vector<Rectangle> &wallsOut, 
                 vector<Rectangle> &windowsOut, vector<Rectangle> &lightsOut, vector<Rectangle> &boxOut, 
                 pair<float, float> &startingPositionOut)
{
    wallsOut.clear();
    windowsOut.clear();
    lightsOut.clear();
    
    int color_type;
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
    
    createLights( Image(width, height, pixelBuffer), scaling, lightsOut);
    
    
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
            
            registerWall(wallsOut, windowsOut, boxOut, pxAbove, pxHere, startX*scaling, y*scaling, endX*scaling, y*scaling);
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
            
            registerWall(wallsOut, windowsOut, boxOut, pxLeft, pxHere, x*scaling, startY*scaling, x*scaling, endY*scaling);
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
        
        if (color != OUTSIDE)
        {
            addHorizontalRect(boxOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), HEIGHT + 0.2); 
            addHorizontalRect(boxOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), - 0.2); 
        }
    }
    

    delete [] pixels;
    //exit(0);
}

