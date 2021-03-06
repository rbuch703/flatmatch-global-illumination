
#include <assert.h>
#include <string.h> //for memcpy
#include <math.h> //for sqrt()

//#include <set>

#include <stdio.h>
#include <stdlib.h>
#include "parseLayout.h"
//#include "png_helper.h"
#include "image.h"
#include "helpers.h"

enum ITEMS { 
    INVALIDATED = 0x00000000, 
    WALL = 0xFF000000, 
    EMPTY = 0xFFFFFFFF, 
    OUTSIDE =  0xFF7F7F7F, 
    DOOR = 0xFFDFDFDF, 
    WINDOW = 0xFF00FF00,
    BALCONY_WINDOW = 0xFFFF7F00,  //has no upper edge (ends at the ceiling)
    BALCONY_DOOR = 0xFFFF0000    //upper edge is WINDOW_HIGH to match the window next to it
};

static const float HEIGHT      = 2.60;
static const float DOOR_HEIGHT = 2.00;
static const float WINDOW_LOW  = 0.85;
static const float WINDOW_HIGH = 2.30;
//static const float WINDOW_HEIGHT = WINDOW_HIGH - WINDOW_LOW;
//static const float TOP_WALL_HEIGHT = HEIGHT - WINDOW_HIGH;

void addWall( RectangleArray *arr, float startX, float startY, float dx, float dy, float min_z, float max_z, const float TILE_SIZE)
{
    insertIntoRectangleArray(arr, createRectangle( startX,startY,min_z,   dx, dy, 0,       0, 0, max_z - min_z, TILE_SIZE));
}

void addFullWall( RectangleArray *arr, float startX, float startY, float dx, float dy, const float TILE_SIZE)
{
    addWall(arr, startX, startY, dx, dy, 0.0f, HEIGHT, TILE_SIZE);
}

void addHorizontalRect(RectangleArray *arr, float startX, float startY, float dx, float dy, float z, const float TILE_SIZE)
{
    insertIntoRectangleArray(arr, createRectangle( startX,startY,z,       dx, 0, 0,        0, dy, 0, TILE_SIZE));
}

void registerWall( RectangleArray *walls, RectangleArray *windows, RectangleArray *box,
                   uint32_t col0, uint32_t col1, float x0, float y0, float x1, float y1, const float TILE_SIZE)
{

    if      (col0 == WALL && col1 == EMPTY) addFullWall(walls, x0, y1, x1 - x0, y0 - y1, TILE_SIZE); //transition from wall to inside area
    else if (col0 == EMPTY && col1 == WALL) addFullWall(walls, x1, y0, x0 - x1, y1 - y0, TILE_SIZE);// transition from inside area to wall

    else if (col0 == WALL && col1 == DOOR) addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, DOOR_HEIGHT, TILE_SIZE); //transition from wall to door frame
    else if (col0 == DOOR && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, DOOR_HEIGHT, TILE_SIZE);// 

    else if (col0 == WALL && col1 == BALCONY_DOOR) addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, WINDOW_HIGH, TILE_SIZE); //transition from wall to door frame
    else if (col0 == BALCONY_DOOR && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, WINDOW_HIGH, TILE_SIZE);// 


    else if (col0 == WALL && col1 == WINDOW) addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, WINDOW_HIGH, TILE_SIZE); //transition from wall to window (frame)
    else if (col0 == WINDOW && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, WINDOW_HIGH, TILE_SIZE); //transition from wall to window (frame)

    else if (col0 == WALL && col1 == BALCONY_WINDOW) addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, HEIGHT, TILE_SIZE); //transition from wall to window (frame)
    else if (col0 == BALCONY_WINDOW && col1 == WALL) addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, HEIGHT, TILE_SIZE); //transition from wall to window (frame)


    else if (col0 == OUTSIDE  && col1 == EMPTY) addFullWall(walls, x0, y1, x1 - x0, y0 - y1, TILE_SIZE); //transition from entrace to inside area
    else if (col0 == EMPTY && col1 == OUTSIDE ) addFullWall(walls, x1, y0, x0 - x1, y1 - y0, TILE_SIZE);// transition from entrace to inside area

    else if (col0 == DOOR && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1, DOOR_HEIGHT, HEIGHT, TILE_SIZE); //transition from door frame to inside area
    else if (col0 == EMPTY && col1 == DOOR) addWall(walls, x1, y0, x0 - x1, y1 - y0, DOOR_HEIGHT, HEIGHT, TILE_SIZE);// transition from door frame to inside area

    else if (col0 == BALCONY_DOOR && col1 == EMPTY) addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_HIGH, HEIGHT, TILE_SIZE); //transition from door frame to inside area
    else if (col0 == EMPTY && col1 == BALCONY_DOOR) addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_HIGH, HEIGHT, TILE_SIZE);// transition from door frame to inside area


    else if (col0 == WALL    && col1 == OUTSIDE) addWall(box, x0, y1, x1 - x0, y0 - y1, -0.2, HEIGHT + 0.2, TILE_SIZE); 
    else if (col0 == OUTSIDE && col1 == WALL   ) addWall(box, x1, y0, x0 - x1, y1 - y0, -0.2, HEIGHT + 0.2, TILE_SIZE); 


    else if (col0 == WINDOW && col1 == EMPTY) { //transition from window to inside area
        addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, WINDOW_LOW, TILE_SIZE); 
        addWall(walls, x0, y1, x1 - x0, y0 - y1, WINDOW_HIGH, HEIGHT, TILE_SIZE); 
    }
    
    else if (col0 == EMPTY && col1 == WINDOW) {
        addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, WINDOW_LOW, TILE_SIZE);
        addWall(walls, x1, y0, x0 - x1, y1 - y0, WINDOW_HIGH, HEIGHT, TILE_SIZE);
    }

    else if (col0 == BALCONY_WINDOW && col1 == EMPTY) { //transition from window to inside area
        addWall(walls, x0, y1, x1 - x0, y0 - y1, 0, WINDOW_LOW, TILE_SIZE); 
    }
    
    else if (col0 == EMPTY && col1 == BALCONY_WINDOW) {
        addWall(walls, x1, y0, x0 - x1, y1 - y0, 0, WINDOW_LOW, TILE_SIZE);
    }


    else if (col0 == OUTSIDE  && col1 == WINDOW) 
    {
        addWall(box,     x1, y0, x0 - x1, y1 - y0, -0.2, WINDOW_LOW, TILE_SIZE);
        addWall(box,     x1, y0, x0 - x1, y1 - y0, WINDOW_HIGH, HEIGHT + 0.2, TILE_SIZE);
        addWall(windows, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, WINDOW_HIGH, TILE_SIZE);
    }
    else if (col0 == WINDOW && col1 == OUTSIDE)  
    {
        addWall(box,     x0, y1, x1 - x0, y0 - y1, -0.2, WINDOW_LOW, TILE_SIZE);
        addWall(box,     x0, y1, x1 - x0, y0 - y1, WINDOW_HIGH, HEIGHT + 0.2, TILE_SIZE);
        addWall(windows, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, WINDOW_HIGH, TILE_SIZE);
    }
    else if (col0 == OUTSIDE  && col1 == BALCONY_WINDOW) 
    {
        addWall(box,     x1, y0, x0 - x1, y1 - y0, -0.2, WINDOW_LOW, TILE_SIZE);
        addWall(windows, x0, y1, x1 - x0, y0 - y1, WINDOW_LOW, HEIGHT, TILE_SIZE);
        addWall(box,     x1, y0, x0 - x1, y1 - y0, HEIGHT, HEIGHT + 0.2, TILE_SIZE);
    }
    else if (col0 == BALCONY_WINDOW && col1 == OUTSIDE)  
    {
        addWall(box,     x0, y1, x1 - x0, y0 - y1, -0.2, WINDOW_LOW, TILE_SIZE);
        addWall(windows, x1, y0, x0 - x1, y1 - y0, WINDOW_LOW, HEIGHT, TILE_SIZE);
        addWall(box,     x0, y1, x1 - x0, y0 - y1, HEIGHT, HEIGHT + 0.2, TILE_SIZE);
    }


}


Point2D getCentralPosition(const Image *img)
{
    Image* tmp = cloneImage(img);

    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
    {
        switch (getImagePixel(tmp, x,y))
        {
            case 0xFFFFFFFF: 
            case 0xFF00FF00: 
            case 0xFFDFDFDF: 
                setImagePixel(tmp, x,y,0);
                break;
                
            default: 
                setImagePixel(tmp, x,y, 1); //walls, outside
                break;
        }
    }

    unsigned int maxDistance = distanceTransform(tmp);
    
    
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
            if (getImagePixel(tmp, x,y) == maxDistance-1)
            {
                freeImage(tmp);
                return (Point2D){x, y};
            }
    
    assert(0);
    freeImage(tmp);
    return (Point2D){-1, -1};
}

int traverseRoom(Image *img, Image *visited, int x, int y, unsigned int *maxDist, Point2DArray *skeletalPoints)
{
    Point2DArray candidates = initPoint2DArray();
    insertIntoPoint2DArray(&candidates, x, y);

    int numPixels = 0;

    while (candidates.numItems)
    {
        Point2D pos = candidates.data[ --candidates.numItems];
        int x = pos.x;
        int y = pos.y;

        if (x < 0 || x >= img->width ) continue;
        if (y < 0 || y >= img->height ) continue;
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
        
        if (! getImagePixel(visited, x-1,y)) insertIntoPoint2DArray(&candidates, x-1, y);
        if (! getImagePixel(visited, x+1,y)) insertIntoPoint2DArray(&candidates, x+1, y);
        if (! getImagePixel(visited, x,y-1)) insertIntoPoint2DArray(&candidates, x, y-1);
        if (! getImagePixel(visited, x,y+1)) insertIntoPoint2DArray(&candidates, x, y+1);

    }
    
    freePoint2DArray(&candidates);
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
    
    /*printf("found room with center at (%d, %d) with distance %d and area %d\n",
           bestCenter.x, bestCenter.y, maxDist, numPixels);*/
           
    float edgeHalfLength = sqrt(numPixels) / 9;//7;

    //maxDist is the maximum distance from maxPos for which it is guaranteed that there is no wall in any direction
    if (edgeHalfLength > maxDist-1)
        edgeHalfLength = maxDist-1;
    
    edgeHalfLength *=scaling;
    float px = bestCenter.x  * scaling;
    float py = bestCenter.y * scaling;
    
    freePoint2DArray(&skeletalPoints);

    insertIntoRectangleArray(lightsOut, 
        createRectangle( px - edgeHalfLength, py - edgeHalfLength, HEIGHT-0.001,
                         2*edgeHalfLength, 0, 0, 0, 2*edgeHalfLength, 0, 0));

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
void createLights(const Image *src, float scaling, RectangleArray* lightsOut)
{
    Image* img = cloneImage(src);

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
    Image *visited = cloneImage(img);
    
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
            setImagePixel(visited, x,y, getImagePixel(img, x ,y) == 1 ? 1 : 0); //WALL

    // The actual steps 3a-e are performed by createLightSourceInRoom(), which also updates the 'visited' map
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
        {
            if (getImagePixel( img, x, y) > 1 && ! getImagePixel(visited, x, y))
                createLightSourceInRoom(img, visited, x, y, scaling, lightsOut);
        }

    freeImage(visited);
    freeImage(img);
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

Geometry* parseLayout(const Image* const src, const float scaling, const float TILE_SIZE)
{
    Image* img = cloneImage(src);  //parseLayout is destructive on the original image, so take a copy

    RectangleArray wallsOut    = initRectangleArray();
    RectangleArray windowsOut  = initRectangleArray();
    RectangleArray lightsOut   = initRectangleArray();
    RectangleArray boxOut      = initRectangleArray();

    Point2D centralPos = getCentralPosition(img);
    
    Image* imgTmp = cloneImage(img);
    
    createLights( imgTmp, scaling, &lightsOut);
    freeImage(imgTmp);
    
    for (int y = 1; y < img->height; y++)
    {
        //cout << "scanning row " << y << endl;
        for (int x = 1; x < img->width;) {
            uint32_t pxAbove = getImagePixel(img, x, y-1);
            uint32_t pxHere =  getImagePixel(img, x, y  );
            if (pxAbove == pxHere)
            {
                x++;
                continue;
            }
                
            float startX = x;
            
            while ( x < img->width && 
                   pxAbove == getImagePixel(img, x, y-1) && 
                   pxHere ==  getImagePixel(img, x, y) )
                x++;
                
            float endX = x;
            
            registerWall(&wallsOut, &windowsOut, &boxOut, pxAbove, pxHere, startX*scaling, y*scaling, endX*scaling, y*scaling, TILE_SIZE);
        }
    }
    //cout << "  == End of horizontal scan, beginning vertical scan ==" << endl;

    for (int x = 1; x < img->width; x++)
    {
        for (int y = 1; y < img->height; ) {
            uint32_t pxLeft = getImagePixel(img, x-1, y);
            uint32_t pxHere = getImagePixel(img, x, y);
            if (pxLeft == pxHere)
            {
                y++;
                continue;
            }
                
            float startY = y;
            
            while (y < img->height && 
                   pxLeft == getImagePixel(img, x-1, y)  && 
                   pxHere == getImagePixel(img, x, y) )
                y++;
                
            float endY = y;
            
            registerWall(&wallsOut, &windowsOut, &boxOut, pxLeft, pxHere, x*scaling, startY*scaling, x*scaling, endY*scaling, TILE_SIZE);
        }
    }

    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
    {
        int xStart = x;
        uint32_t color = getImagePixel(img, x, y);
        if (color == INVALIDATED)
            continue;
        
        while (x+1 < img->width && getImagePixel(img, x+1, y) == color) 
            x++;
            
        int xEnd = x;
        
        int yEnd;
        for (yEnd = y + 1; yEnd < img->height; yEnd++)
        {
            int rowWithIdenticalColor = 1;
            for (int xi = xStart; xi <= xEnd && rowWithIdenticalColor; xi++)
                rowWithIdenticalColor &= (getImagePixel(img, xi, yEnd) == color);
            
            if (! rowWithIdenticalColor)
                break;
        }
        
        yEnd--;

        for (int yi = y; yi <= yEnd; yi++)
            for (int xi = xStart; xi <= xEnd; xi++)
                setImagePixel(img, xi, yi, INVALIDATED);

        yEnd +=1;   //cover the area to the end of the pixel, not just to the start
        xEnd +=1;        
        switch (color)
        {
            case WINDOW: //window --> create upper and lower window frame
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), WINDOW_LOW, TILE_SIZE); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), WINDOW_HIGH, TILE_SIZE); 
                break;

            case BALCONY_WINDOW: //window --> create upper and lower window frame
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), WINDOW_LOW, TILE_SIZE); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), HEIGHT, TILE_SIZE); 
                break;


            case EMPTY: //empty floor --> create floor and ceiling
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0, TILE_SIZE); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), HEIGHT, TILE_SIZE); 
                break;
                
            case DOOR: // --> create floor and upper door frame
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0, TILE_SIZE); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), DOOR_HEIGHT, TILE_SIZE); 
                break;
                
            case BALCONY_DOOR: // --> create floor and upper door frame
                addHorizontalRect(&wallsOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), 0, TILE_SIZE); 
                addHorizontalRect(&wallsOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), WINDOW_HIGH, TILE_SIZE);
                break;
        }
        
        if (color != OUTSIDE)
        {
            addHorizontalRect(&boxOut, scaling*xEnd,   scaling*y, scaling*(xStart - xEnd), scaling*(yEnd - y), HEIGHT + 0.2, TILE_SIZE); 
            addHorizontalRect(&boxOut, scaling*xStart, scaling*y, scaling*(xEnd - xStart), scaling*(yEnd - y), - 0.2, TILE_SIZE); 
        }
    }

    Geometry *geo = (Geometry*)malloc(sizeof(Geometry));
    *geo = (Geometry){ 
        .windows  = convertToAlignedArray(windowsOut.data, windowsOut.numItems),
        .lights   = convertToAlignedArray(lightsOut.data,  lightsOut.numItems),
        .walls    = convertToAlignedArray(wallsOut.data,   wallsOut.numItems),
        .boxWalls = convertToAlignedArray(boxOut.data,     boxOut.numItems),
        .numWindows=  windowsOut.numItems,
        .numLights =  lightsOut.numItems,
        .numWalls =   wallsOut.numItems,
        .numBoxWalls= boxOut.numItems,
        .width = img->width, 
        .height= img->height, 
        .startingPositionX = centralPos.x * scaling,
        .startingPositionY = centralPos.y * scaling,
        .numTexels = 0,
        .texels = NULL };

    freeImage(img);

    for ( int i = 0; i < geo->numWalls; i++)
    {
        geo->walls[i].lightmapSetup.s[0] = geo->numTexels;
        //objects[i].lightNumTiles = getNumTiles(&objects[i]);
        geo->numTexels += getNumMipmapTexels(&geo->walls[i]);
    }
        
    printf( "[DBG] allocating %.2fMB for texels\n", geo->numTexels * sizeof(Vector3)/1000000.0);
    if (geo->numTexels * sizeof(Vector3) > 1000*1000*1000)
    {
        printf("[Err] Refusing to allocate more than 1GB for texels, as this would crash most GPUs. Exiting ...\n");
        exit(0);
    }

    if (0 != posix_memalign( (void**) &geo->texels, 16, geo->numTexels * sizeof(Vector3)))
    {
        printf("[Err] aligned memory allocation failed, exiting ...\n");
        exit(0);
    }
    
    for (int i = 0; i < geo->numTexels; i++)
        geo->texels[i] = vec3(0,0,0);
    
    return geo;
}

static int toJsonString(int width, int height, uint8_t *collisionMap, char *out, int outSize)
{
    #define PRINT( ...) print(out, &outPos, outSize, __VA_ARGS__)
    int outPos = 0;
    int runLength = 0;
    int runIsPassable = 0; /* The first run is specified to be impassable.*/
    int count = 0;

    PRINT("[");
    
    for (int i = 0; i < width*height; i++)
    {
        int isPassable = (collisionMap[i] != 0x00);
                          
        if (isPassable == runIsPassable)
        {
            runLength++;
            continue;    
        }
        
        PRINT("%d,", runLength);
        if (++count % 30 == 0)
            PRINT("\n");
        
        runLength = 1;
        runIsPassable = isPassable;
    }

    PRINT("%d]\n", runLength);    
    #undef PRINT
    return outPos;
}

void dilate(int width, int height, uint8_t *collisionMap, int radius)
{

    uint8_t *tmp = (uint8_t*) malloc(width*height *sizeof(uint8_t));
    memcpy(tmp, collisionMap, width*height *sizeof(uint8_t));
    
    // dilate the impassable (numerical value 0) area by 'radius' pixels
    
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
    {
        if (tmp[y*width + x])
            continue;
            
        for (int dy = -radius; dy <= radius; dy++)
            for (int dx = -radius; dx <= radius; dx++)
                if (y+dy >= 0 && y+dy < height &&
                    x+dx >= 0 && x+dx < width)
                    collisionMap[ (y+dy)*width + (x+dx)] = 0;
    }
        
    
    free(tmp);
}

/*output format: This method outputs the collision map as a b/w image. Black pixels are impassable, white ones
 *               are traversable. To save storage space and transfer bandwith, the map is RLE-encoded as follows:
 *               - the result is a single array on run lengths
 *               - each run is given just by its length
 *               - run indices are zero-based (corresponding to the Javascript array indices)
 *               - every run with an even run index is impassable, those with odd indices are traversable
 */

char* buildCollisionMap(const Image* const img)
{
    uint8_t *collisionMap = (uint8_t*) malloc(img->width * img->height *sizeof(uint8_t));
    for (int i = 0; i < img->width * img->height; i++)
    {
        uint32_t val = img->data[i];
            
        // EMPTY, DOOR, and BALCONY_DOOR are passable
        collisionMap[i] = (val == 0xFFFFFFFF || val == 0xFFDFDFDF || val == 0xFFFF0000) ?
            255 : 0;
    }

    dilate(img->width, img->height, collisionMap, 5);
    /*
    FILE* f = fopen("collision.raw", "wb");
    fwrite(collisionMap, width*height*sizeof(uint8_t), 1, f);
    fclose(f);*/
    
    int nBytes = toJsonString(img->width, img->height, collisionMap, NULL, 0) + 1;
    char* str = (char*)malloc(nBytes);
    str[0] = '\0';
    toJsonString(img->width, img->height, collisionMap, str, nBytes);
    
    free(collisionMap);
    
    return str;
}
