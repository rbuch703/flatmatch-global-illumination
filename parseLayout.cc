
#include <assert.h>
#include <string.h> //for memcpy
#include <math.h> //for sqrt()

#include <set>

#include <stdio.h>
#include "parseLayout.h"
#include "png_helper.h"
#include "image.h"

#include <fstream>
#include <sstream>

static const uint8_t INVALIDATED = 0x0;
static const uint8_t WALL = 0x1;
static const uint8_t EMPTY = 0x2;
static const uint8_t OUTSIDE =  0x3;
static const uint8_t DOOR = 0x4;
static const uint8_t WINDOW = 0x5;
static const uint8_t BALCONY_WINDOW = 0x6;  //has no upper edge (ends at the ceiling)
static const uint8_t BALCONY_DOOR = 0x7;    //upper edge is WINDOW_HIGH to match the window next to it


static const float HEIGHT      = 2.60;
static const float DOOR_HEIGHT = 2.00;
static const float WINDOW_LOW  = 0.85;
static const float WINDOW_HIGH = 2.30;
//static const float WINDOW_HEIGHT = WINDOW_HIGH - WINDOW_LOW;
//static const float TOP_WALL_HEIGHT = HEIGHT - WINDOW_HIGH;

typedef struct {
    Rectangle *data;
    cl_int numItems;
    cl_int maxNumItems;
} RectangleArray;


RectangleArray initRectangleArray()
{
    RectangleArray res = {.data = (Rectangle*)malloc ( sizeof(Rectangle) * 64), 
                        .numItems = 0, 
                        .maxNumItems = 64};
    return res;
};

void freeRectangleArray(RectangleArray *arr)
{
    free(arr->data);
    arr->data = NULL;
}

void resizeRectangleArray(RectangleArray *arr, int newSize)
{
    arr->data = (Rectangle*)realloc(arr->data, newSize*sizeof(Rectangle));
    arr->maxNumItems = newSize;
    if (arr->numItems > arr->maxNumItems)   // array was just shortened
        arr->numItems = arr->maxNumItems;
}

void insertIntoRectangleArray(RectangleArray *arr, Rectangle rect)
{
    if (arr->numItems == arr->maxNumItems)
        resizeRectangleArray( arr, arr->maxNumItems*2);
        
    arr->data[arr->numItems++] = rect;
}




void addWall( RectangleArray *arr, float startX, float startY, float dx, float dy, float min_z, float max_z)
{
    insertIntoRectangleArray(arr, createRectangle( startX,startY,min_z,   dx, dy, 0,       0, 0, max_z - min_z));
}

void addFullWall( RectangleArray *arr, float startX, float startY, float dx, float dy)
{
    addWall(arr, startX, startY, dx, dy, 0.0f, HEIGHT);
}

void addHorizontalRect(RectangleArray *arr, float startX, float startY, float dx, float dy, float z)
{
    insertIntoRectangleArray(arr, createRectangle( startX,startY,z,       dx, 0, 0,        0, dy, 0));
}

void registerWall( RectangleArray *walls, RectangleArray *windows, RectangleArray *box,
                   uint32_t col0, uint32_t col1, float x0, float y0, float x1, float y1)
{

    if      (col0 == WALL && col1 == EMPTY) addFullWall(walls, x0, y1, x1 - x0, y0 - y1); //transition from wall to inside area
    else if (col0 == EMPTY && col1 == WALL) addFullWall(walls, x1, y0, x0 - x1, y1 - y0);// transition from inside area to wall

    else if (col0 == WALL && col1 == DOOR) addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, DOOR_HEIGHT); //transition from wall to door frame
    else if (col0 == DOOR && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, DOOR_HEIGHT);// 

    else if (col0 == WALL && col1 == BALCONY_DOOR) addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, WINDOW_HIGH); //transition from wall to door frame
    else if (col0 == BALCONY_DOOR && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, WINDOW_HIGH);// 


    else if (col0 == WALL && col1 == WINDOW) addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)
    else if (col0 == WINDOW && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, WINDOW_HIGH); //transition from wall to window (frame)

    else if (col0 == WALL && col1 == BALCONY_WINDOW) addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, HEIGHT); //transition from wall to window (frame)
    else if (col0 == BALCONY_WINDOW && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, HEIGHT); //transition from wall to window (frame)


    else if (col0 == OUTSIDE  && col1 == EMPTY) addFullWall(walls, x0, y1, x1 - x0, y0 - y1); //transition from entrace to inside area
    else if (col0 == EMPTY && col1 == OUTSIDE ) addFullWall(walls, x1, y0, x0 - x1, y1 - y0);// transition from entrace to inside area

    else if (col0 == DOOR && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1, DOOR_HEIGHT, HEIGHT); //transition from door frame to inside area
    else if (col0 == EMPTY && col1 == DOOR) addWall(walls, x1, y0, x0 - x1, y1 - y0, DOOR_HEIGHT, HEIGHT);// transition from door frame to inside area

    else if (col0 == BALCONY_DOOR && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_HIGH, HEIGHT); //transition from door frame to inside area
    else if (col0 == EMPTY && col1 == BALCONY_DOOR) addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_HIGH, HEIGHT);// transition from door frame to inside area


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

    else if (col0 == BALCONY_WINDOW && col1 == EMPTY) { //transition from window to inside area
        addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, WINDOW_LOW); 
    }
    
    else if (col0 == EMPTY && col1 == BALCONY_WINDOW) {
        addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, WINDOW_LOW);
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
    else if (col0 == OUTSIDE  && col1 == BALCONY_WINDOW) 
    {
        addWall(box,     x1, y0, x0 - x1, y1 - y0, -0.2, WINDOW_LOW);
        addWall(windows, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, HEIGHT);
        addWall(box,     x1, y0, x0 - x1, y1 - y0, HEIGHT, HEIGHT + 0.2);
    }
    else if (col0 == BALCONY_WINDOW && col1 == OUTSIDE)  
    {
        addWall(box,     x0, y1, x1 - x0, y0 - y1, -0.2, WINDOW_LOW);
        addWall(windows, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, HEIGHT);
        addWall(box,     x0, y1, x1 - x0, y0 - y1, HEIGHT, HEIGHT + 0.2);
    }


}


Point2D getCentralPosition(uint32_t *pixelBuffer, int width, int height)
{
    
    Image img = {.width = width, .height=height, .data = (uint32_t*)malloc(width*height*sizeof(uint32_t))};
    memcpy(img.data, pixelBuffer, width*height*sizeof(uint32_t));

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
    {
        switch (getImagePixel(&img, x,y))
        {
            case 0xFFFFFFFF: 
            case 0xFF00FF00: 
            case 0xFFDFDFDF: 
                setImagePixel(&img, x,y,0);
                break;
                
            default: 
                setImagePixel(&img, x,y, 1); //walls, outside
                break;
        }
    }

    unsigned int maxDistance = distanceTransform(&img);
    
    
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            if (getImagePixel(&img, x,y) == maxDistance-1)
            {
                freeImage(&img);
                return (Point2D){x, y};
            }
    
    assert(0);
    freeImage(&img);
    return (Point2D){-1, -1};
}

bool lessThan( Point2D a, Point2D b) { return a.x < b.x || (a.x == b.x && a.y < b.y);}

int traverseRoom(Image *img, Image *visited, int x, int y, unsigned int *maxDist, Point2DArray *skeletalPoints)
{
    std::set< Point2D, bool(*)( Point2D a, Point2D b)>  candidates(lessThan);
    std::set< Point2D, bool(*)( Point2D a, Point2D b)>  visitedPoints(lessThan);
    
    candidates.insert( (Point2D){x,y} );

    int numPixels = 0;

    while (! candidates.empty())
    {
        Point2D pos = * (candidates.begin());
        int x = pos.x;
        int y = pos.y;
        visitedPoints.insert(pos);
        candidates.erase(pos);

        if (x < 0 || x >= img->width ) return 0;
        if (y < 0 || y >= img->height ) return 0;
        assert( img->width  == visited->width  );
        assert( img->height == visited->height );

        if (getImagePixel(img, x, y) == 0) continue; //stepped on a wall
        if (getImagePixel(visited, x,y))   continue; //already been here
        setImagePixel(visited, x, y, 2);
        numPixels += 1;

        if ( getImagePixel(img, x,y) >= getImagePixel(img, x+1, y) && 
             getImagePixel(img, x,y) >= getImagePixel(img, x-1, y) &&
             getImagePixel(img, x,y) >= getImagePixel(img, x, y+1) && 
             getImagePixel(img, x,y) >= getImagePixel(img, x, y-1))
        {
            insertIntoPoint2DArray(skeletalPoints, x, y);
            setImagePixel(visited, x, y, 3);
        }

        
        if ( getImagePixel(img, x, y) > *maxDist)
            *maxDist = getImagePixel(img, x, y);
        
        if (visitedPoints.count( Point2D{x-1, y}) == 0) candidates.insert( Point2D{x-1,y });
        if (visitedPoints.count( Point2D{x+1, y}) == 0) candidates.insert( Point2D{x+1,y });
        if (visitedPoints.count( Point2D{x  , y-1}) == 0) candidates.insert( Point2D{x  ,y-1});
        if (visitedPoints.count( Point2D{x  , y+1}) == 0) candidates.insert( Point2D{x  ,y+1});
    }
    
    return numPixels;
}

int sqr(int a) { return a*a;}

void createLightSourceInRoom(Image *img, Image *visited, int roomX, int roomY, float scaling, RectangleArray *lightsOut)
{
    assert( getImagePixel(img, roomX, roomY) > 1);
    
    unsigned int maxDist = 1;

    Point2DArray skeletalPoints = initPoint2DArray();
    int numPixels = traverseRoom(img, visited, roomX, roomY, &maxDist, &skeletalPoints);
    assert(skeletalPoints.numItems > 0);

    int min_x = skeletalPoints.data[0].x;
    int max_x = skeletalPoints.data[0].x;
    int min_y = skeletalPoints.data[0].y;
    int max_y = skeletalPoints.data[0].y;
    
    for (unsigned int i = 0; i < skeletalPoints.numItems; i++)
    {
        int x = skeletalPoints.data[i].x;
        int y = skeletalPoints.data[i].y;
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        
        if ( getImagePixel(img, x,y)  >= 0.9*maxDist)
            setImagePixel(visited, x,y,4);
    }
    
    int mid_x = (min_x + max_x) / 2;
    int mid_y = (min_y + max_y) / 2;
    //cout << "Mid point is (" << mid_x << ", " << mid_y << ")" << endl;
    
    Point2D bestCenter = skeletalPoints.data[0];
    int bestDist = ( sqr( bestCenter.x - mid_x) + sqr( bestCenter.y - mid_y));
    for (unsigned int i = 0; i < skeletalPoints.numItems; i++)
    {
        int x = skeletalPoints.data[i].x;
        int y = skeletalPoints.data[i].y;
        int dist = ( sqr( x - mid_x) + sqr( y - mid_y));
        if (dist < bestDist)
        {
            bestDist = dist;
            bestCenter = skeletalPoints.data[i];
        }
    }    
    setImagePixel(visited, bestCenter.x, bestCenter.y, 5);
    
    printf("found room with center at (%d, %d) with distance %d and area %d\n",
           bestCenter.x, bestCenter.y, maxDist, numPixels);
           
    float edgeHalfLength = sqrt(numPixels) / 10;//7;

    //maxDist is the maximum distance from maxPos for which it is guaranteed that there is no wall in any direction
    if (edgeHalfLength > maxDist-1)
        edgeHalfLength = maxDist-1;
    
    edgeHalfLength *=scaling;
    float px = bestCenter.x  * scaling;
    float py = bestCenter.y * scaling;
    
    freePoint2DArray(&skeletalPoints);

    insertIntoRectangleArray(lightsOut, 
        createRectangle( px - edgeHalfLength, py - edgeHalfLength, HEIGHT-0.001,
                         2*edgeHalfLength, 0, 0, 0, 2*edgeHalfLength, 0));

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
 
 /*Note: this operation is destructive on 'img' */
void createLights(Image *img, float scaling, RectangleArray* lightsOut)
{

    //Step 1: Flood-fill rooms adjacent to windows with window color, to mark them as not requiring additional lighting    
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
    {
        if (getImagePixel(img,x, y) == 0xFF00FF00) //window
        {
            if (getImagePixel(img,x-1, y  ) == 0xFFFFFFFF) floodFillImage(img, x-1, y,   0xFF00FF00, 0xFFFFFFFF);
            if (getImagePixel(img,x+1, y  ) == 0xFFFFFFFF) floodFillImage(img, x+1, y,   0xFF00FF00, 0xFFFFFFFF);
            if (getImagePixel(img,x,   y-1) == 0xFFFFFFFF) floodFillImage(img, x,   y-1, 0xFF00FF00, 0xFFFFFFFF);
            if (getImagePixel(img,x,   y+1) == 0xFFFFFFFF) floodFillImage(img, x,   y+1, 0xFF00FF00, 0xFFFFFFFF);
        }
    }
    saveImageAs(img, "filled.png");

    //prepare map for distance transform: only empty space inside the apartment is considered ("0")
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
            setImagePixel(img, x, y, (getImagePixel(img, x,y) == 0xFFFFFFFF) ? 0 : 1);

    //Step 2, the actual distance transform
    distanceTransform(img);

    // prepare map of pixels that have already been considered for placing lights (initially only those not used in the distance transform
    Image visited = {img->width, img->height, (uint32_t*)malloc(img->width * img->height * sizeof(uint32_t))};
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
            setImagePixel(&visited, x,y, getImagePixel(img, x ,y) == 1 ? 1 : 0); //WALL

    // The actual steps 3a-e are performed by createLightSourceInRoom(), which also updates the 'visited' map
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
        {
            if (getImagePixel( img, x, y) > 1 && ! getImagePixel(&visited, x, y))
                createLightSourceInRoom(img, &visited, x, y, scaling, lightsOut);
        }

    freeImage(&visited);
}

//these Rectangle arrays are to be passed to OpenCL --> have to be aligned to 16 byte boundary
Rectangle* convertToAlignedArray( Rectangle* rectsIn, int numRects)
{
    
    Rectangle* res;
    if (0 != posix_memalign((void**)&res, 16, numRects * sizeof(Rectangle))) 
    {
        printf("[Err] aligned memory allocation failed, exiting ...\n");
        exit(0);
    }

    memcpy(res, rectsIn, numRects * sizeof(Rectangle));
    free(rectsIn);
    return res;
}

static Geometry parseLayoutCore(uint8_t *src, int width, int height, int colorType, const float scaling)
{
    RectangleArray wallsOut    = initRectangleArray();
    RectangleArray windowsOut  = initRectangleArray();
    RectangleArray lightsOut   = initRectangleArray();
    RectangleArray boxOut      = initRectangleArray();

    
    uint32_t *pixelBuffer = (uint32_t*)src;
    if (colorType == PNG_COLOR_TYPE_RGB)
    {
        //uint8_t *src = (uint8_t*)pixelBuffer;
        pixelBuffer = (uint32_t*)malloc( width * height * sizeof(uint32_t));
        
        for (int i = 0; i < width * height; i++)
            pixelBuffer[i] =
                0xFF000000 | src[i*3] | (src[i*3+1] << 8) | (src[i*3+2] << 16);
        
        //free(src);
        colorType = PNG_COLOR_TYPE_RGBA;
    } else


    assert (colorType == PNG_COLOR_TYPE_RGBA);

    uint8_t *pixels = new uint8_t[width * height];
    for (int i = 0; i < width * height; i++)
    {
        switch (pixelBuffer[i])
        {
            case 0x00000000: pixels[i] = INVALIDATED; break;
            case 0xFF000000: pixels[i] = WALL; break;
            case 0xFFFFFFFF: pixels[i] = EMPTY; break;
            case 0xFF7F7F7F: pixels[i] = OUTSIDE; break;
            case 0xFF00FF00: pixels[i] = WINDOW; break;
            case 0xFFDFDFDF: pixels[i] = DOOR; break;
            case 0xFFFF0000: pixels[i] = BALCONY_DOOR; break;
            case 0xFFFF7F00: pixels[i] = BALCONY_WINDOW; break;
        }
    }

    Point2D centralPos = getCentralPosition(pixelBuffer, width, height);
    
    Image imgTmp = {width, height, (uint32_t*)malloc(width*height*sizeof(uint32_t))};
    memcpy(imgTmp.data, pixelBuffer, width*height*sizeof(uint32_t));
    
    createLights( &imgTmp, scaling, &lightsOut);
    freeImage(&imgTmp);
    
    if (pixelBuffer != (void*)src) //if they are equal, no memory was allocated for pixelBuffer, and none should be freed
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
            
            registerWall(&wallsOut, &windowsOut, &boxOut, pxAbove, pxHere, startX*scaling, y*scaling, endX*scaling, y*scaling);
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
            
            registerWall(&wallsOut, &windowsOut, &boxOut, pxLeft, pxHere, x*scaling, startY*scaling, x*scaling, endY*scaling);
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
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), WINDOW_LOW); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), WINDOW_HIGH); 
                break;

            case BALCONY_WINDOW: //window --> create upper and lower window frame
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), WINDOW_LOW); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), HEIGHT); 
                break;


            case EMPTY: //empty floor --> create floor and ceiling
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), HEIGHT); 
                break;
                
            case DOOR: // --> create floor and upper door frame
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), DOOR_HEIGHT); 
                break;
                
            case BALCONY_DOOR: // --> create floor and upper door frame
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), WINDOW_HIGH);
                break;
        }
        
        if (color != OUTSIDE)
        {
            addHorizontalRect(&boxOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), HEIGHT + 0.2); 
            addHorizontalRect(&boxOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), - 0.2); 
        }
    }

    delete [] pixels;

    Geometry geo = { 
        .windows  = convertToAlignedArray(windowsOut.data, windowsOut.numItems),
        .lights   = convertToAlignedArray(lightsOut.data,  lightsOut.numItems),
        .walls    = convertToAlignedArray(wallsOut.data,   wallsOut.numItems),
        .boxWalls = convertToAlignedArray(boxOut.data,     boxOut.numItems),
        .numWindows=  windowsOut.numItems,
        .numLights =  lightsOut.numItems,
        .numWalls =   wallsOut.numItems,
        .numBoxWalls= boxOut.numItems,
        .width = width, 
        .height= height, 
        .startingPositionX = centralPos.x * scaling,
        .startingPositionY = centralPos.y * scaling,
        .numTexels = 0,
        .texels = NULL };

    for ( int i = 0; i < geo.numWalls; i++)
    {
        geo.walls[i].lightmapSetup.s[0] = geo.numTexels;
        //objects[i].lightNumTiles = getNumTiles(&objects[i]);
        geo.numTexels += getNumTiles(&geo.walls[i]);
    }
        
    printf( "[DBG] allocating %fMB for texels\n", geo.numTexels * sizeof(Vector3)/1000000.0);
    if (geo.numTexels * sizeof(Vector3) > 1000*1000*1000)
    {
        printf("[Err] Refusing to allocate more than 1GB for texels, this would crash most GPUs. Exiting ...\n");
        exit(0);
    }

    if (0 != posix_memalign( (void**) &geo.texels, 16, geo.numTexels * sizeof(Vector3)))
    {
        printf("[Err] aligned memory allocation failed, exiting ...\n");
        exit(0);
    }
    
    for (int i = 0; i < geo.numTexels; i++)
        geo.texels[i] = vec3(0,0,0);
    
    return geo;
}


Geometry parseLayout(const char* const filename, const float scaling)
{
    int colorType, width, height;
    uint8_t *pixelBuffer;
    read_png_file(filename, &width, &height, &colorType, (uint8_t**)&pixelBuffer );
    
    Geometry geo = parseLayoutCore(pixelBuffer, width, height, colorType, scaling);
    free(pixelBuffer);
    return geo;
}

Geometry parseLayoutMem(const uint8_t *data, int dataSize, const float scaling)
{
    int colorType, width, height;
    uint8_t *pixelBuffer;
    read_png_from_memory(data, dataSize, &width, &height, &colorType, (uint8_t**)&pixelBuffer );
    
    Geometry geo = parseLayoutCore(pixelBuffer, width, height, colorType, scaling);
    free(pixelBuffer);
    return geo;
}

Geometry* parseLayoutStatic(const char* filename, float scale)
{
    static Geometry geo = { .windows= NULL, .lights=NULL, .walls = NULL, .boxWalls = NULL,
    .numWindows = 0, .numLights = 0, .numWalls = 0, .numBoxWalls = 0, .width = 0, .height = 0,
    .startingPositionX = 0, .startingPositionY = 0, .numTexels= 0, .texels = NULL };
    
    freeGeometry(geo);  // in case of leftover data from a previous call to this method
    geo = parseLayout(filename, scale);
    
    return &geo;
}

Geometry* parseLayoutStaticMem(const uint8_t* data, int dataSize, float scale)
{
    static Geometry geo = { .windows= NULL, .lights=NULL, .walls = NULL, .boxWalls = NULL,
    .numWindows = 0, .numLights = 0, .numWalls = 0, .numBoxWalls = 0, .width = 0, .height = 0,
    .startingPositionX = 0, .startingPositionY = 0, .numTexels= 0, .texels = NULL };
    
    freeGeometry(geo);  // in case of leftover data from a previous call to this method
    geo = parseLayoutMem(data, dataSize, scale);
    
    return &geo;
}



std::ostream& operator<<(std::ostream &os, const Vector3 &vec)
{
    os <<  "[" << vec.s[0] << ", " << vec.s[1] << ", " << vec.s[2] << "]";
    return os;
}

void writeJsonOutputStream(Geometry geo, std::ostream &jsonGeometry)
{

    jsonGeometry << "{" << std::endl;
    jsonGeometry << "\"startingPosition\" : [" << geo.startingPositionX << ", " << geo.startingPositionY << "]," <<  std::endl;
    jsonGeometry << "\"layoutImageSize\" : [" << geo.width << ", " << geo.height << "]," <<  std::endl;

    jsonGeometry << "\"geometry\" : [" << std::endl;
    for ( int i = 0; i < geo.numWalls; i++)
    {
        //{ pos: [1,2,3], width: [4,5,6], height: [7,8,9], texture_id: 10, isWindow: false};
        jsonGeometry << "  { \"pos\": " << geo.walls[i].pos <<
                        ", \"width\": " << geo.walls[i].width << 
                        ", \"height\": "<< geo.walls[i].height <<
                        ", \"textureId\": " << i << "}";
        if (i+1 < geo.numWalls)
            jsonGeometry << ", ";
        jsonGeometry << std::endl;
        
    }

    jsonGeometry << "]," << std::endl;
    jsonGeometry << "\"box\": [" << std::endl;

    for (int i = 0; i < geo.numBoxWalls; i++)
    {
        jsonGeometry << "  { \"pos\": " << geo.boxWalls[i].pos <<
                        ", \"width\": " << geo.boxWalls[i].width << 
                        ", \"height\": "<< geo.boxWalls[i].height << "}";
        if (i + 1 < geo.numBoxWalls)
            jsonGeometry << ", ";
        jsonGeometry << std::endl;
    }


    jsonGeometry << "]" << std::endl;
    jsonGeometry << "}" << std::endl;
}

void writeJsonOutput(Geometry geo, const char*filename)
{
    std::ofstream jsonGeometry(filename);
    writeJsonOutputStream(geo, jsonGeometry);
    jsonGeometry.close();
}

char* getJsonFromLayout(const char* const filename, float scaling)
{
    Geometry geo = parseLayout(filename, scaling);
    std::stringstream ss;
    writeJsonOutputStream(geo, ss);

    free (geo.walls);
    free (geo.boxWalls);
    free (geo.windows);
    free (geo.lights);

    return strdup( ss.str().c_str());
}


char* getJsonFromLayoutMem(const uint8_t *data, int dataSize,float scaling)
{
    int width, height, colorType;
    uint8_t *pixelBuffer;
    
    read_png_from_memory(data, dataSize, &width, &height, &colorType, &pixelBuffer );
    Geometry geo = parseLayoutCore(pixelBuffer, width, height, colorType, scaling);
    free(pixelBuffer);
    std::stringstream ss;
    writeJsonOutputStream(geo, ss);
    
    free (geo.walls);
    free (geo.boxWalls);
    free (geo.windows);
    free (geo.lights);
    
    return strdup( ss.str().c_str());
}

