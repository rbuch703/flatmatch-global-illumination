
#include "image.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "png_helper.h"

static int min(int x, int y ) { return x < y ? x : y;}
static int max(int x, int y ) { return x > y ? x : y;}


Point2DArray initPoint2DArray()
{
    return (Point2DArray){
        .data = (Point2D*)malloc ( sizeof(Point2D) * 64), 
        .numItems = 0, .maxNumItems = 64};
};

void resizePoint2DArray(Point2DArray *arr, int newSize)
{
    arr->data = (Point2D*)realloc(arr->data, newSize*sizeof(Point2D));
    arr->maxNumItems = newSize;
    if (arr->numItems > arr->maxNumItems)   // array was just shortened
        arr->numItems = arr->maxNumItems;
}

void insertIntoPoint2DArray(Point2DArray *arr, int x, int y)
{
    if (arr->numItems == arr->maxNumItems)
    {
        resizePoint2DArray( arr, arr->maxNumItems*2);
    }
        
    assert (arr->numItems < arr->maxNumItems);
    arr->data[arr->numItems++] = ( Point2D ){.x = x, .y = y};
}

void freePoint2DArray(Point2DArray *arr)
{
//    printf("freeing point2d list with %d entries\n", arr->maxNumItems);
    free(arr->data);
    arr->data = NULL;
}

void freeImage(Image *img)
{
    free(img->data);
    img->data = NULL;
    free(img);
}

int clampInt(int val, int min, int max)
{
    assert(min <= max);
    return val < min ? min : (val > max ? max : val);
}
    
uint32_t getImagePixel(const Image* const img, int x, int y)
{
    x = clampInt(x, 0, img->width-1);
    y = clampInt(y, 0, img->height-1);
    return img->data[y*img->width+x];
}

int setImagePixel(Image* img, int x, int y, uint32_t val)
{
    if ( x < 0) return 0;
    if ( x >= img->width) return 0;
    if ( y < 0) return 0;
    if ( y >= img->height) return 0;
    
    img->data[y*img->width+x] = val;
    return 1;
}

int anyEmptyNeighbor(Image* img, int x, int y)
{
    for (int dx = -1; dx <= +1; dx++)
        for (int dy = -1; dy <= +1; dy++)
            if (getImagePixel(img, x+dx, y+dy) == 0)
                return 1;
    return 0;
}

unsigned int distanceTransform(Image *img)
{

    Point2DArray openList = initPoint2DArray();
    unsigned int distance = 1;
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
    {
        if ( getImagePixel(img, x,y) == distance && anyEmptyNeighbor(img, x, y))
            insertIntoPoint2DArray(&openList, x, y);
    }

    Point2DArray newOpenList = initPoint2DArray();
    while (openList.numItems)
    {
        //cout << "open list for distance " << distance << " contains " << openList.size() << " entries" << endl;

        for (unsigned int i = 0; i < openList.numItems; i++)
        {
            
            int xTmp = openList.data[i].x;
            int yTmp = openList.data[i].y;
            
            for (int x = max(xTmp-1, 0); x <= min(xTmp + 1, img->width - 1); x++)
                for (int y = max(yTmp - 1, 0); y <= min(yTmp + 1, img->height-1); y++)
                {
                    if (x == xTmp && y == yTmp)
                        continue;
                        
                    if ( getImagePixel(img, x, y) == 0)
                    {
                        setImagePixel(img, x, y, distance+1);
                        insertIntoPoint2DArray(&newOpenList, x, y);
                    }
                }
        }
        
        Point2DArray tmp = openList;
        openList = newOpenList;
        newOpenList = tmp;

        newOpenList.numItems = 0;
        distance += 1;
    }
    
    free(openList.data);
    free(newOpenList.data);

    //the while loop loops until the open list for a distance is completely empty,
    //i.e. until a distance is reached for which not a single pixel exists.
    //so the actual maximum distance returned here is one less than the distance
    //for which the loop terminated
    return distance - 1;

}

void floodFillImage(Image *img, int x, int y, uint32_t value, uint32_t background)
{
    
    Point2DArray candidates = initPoint2DArray();
    insertIntoPoint2DArray(&candidates, x, y);    
    
    while (candidates.numItems)
    {
        int x = candidates.data[candidates.numItems-1].x;
        int y = candidates.data[candidates.numItems-1].y;
        candidates.numItems -= 1;

        if (x < 0 || x >= img->width) continue;
        if (y < 0 || y >= img->height) continue;
    
        if (getImagePixel(img, x, y) != background)
            continue;
            
        setImagePixel(img, x, y, value);
        
        if (getImagePixel(img, x - 1, y - 1) == background) insertIntoPoint2DArray(&candidates, x - 1, y - 1 );
        if (getImagePixel(img, x    , y - 1) == background) insertIntoPoint2DArray(&candidates, x    , y - 1 );
        if (getImagePixel(img, x + 1, y - 1) == background) insertIntoPoint2DArray(&candidates, x + 1, y - 1 );

        if (getImagePixel(img, x - 1, y    ) == background) insertIntoPoint2DArray(&candidates, x - 1, y     );
        //if (getImagePixel(img, x   1, y - 1) == background) insertIntoPoint2DArray(&candidates, x    , y - 1 );
        if (getImagePixel(img, x + 1, y    ) == background) insertIntoPoint2DArray(&candidates, x + 1, y     );

        if (getImagePixel(img, x - 1, y + 1) == background) insertIntoPoint2DArray(&candidates, x - 1, y + 1 );
        if (getImagePixel(img, x    , y + 1) == background) insertIntoPoint2DArray(&candidates, x    , y + 1 );
        if (getImagePixel(img, x + 1, y + 1) == background) insertIntoPoint2DArray(&candidates, x + 1, y + 1 );

    }
    
    freePoint2DArray(&candidates);
}

void saveImageAs(Image *img, const char* filename)
{
    write_png_file( filename, img->width, img->height, PNG_COLOR_TYPE_RGBA, (uint8_t*)img->data);
}
    
/*int getHeight() const { return height;}
int getWidth()  const { return width;}*/

static uint32_t* convertRgbToRgba(uint8_t *rgbData, int numElements)
{
    //uint8_t *src = (uint8_t*)pixelBuffer;
    uint32_t *data = (uint32_t*)malloc( numElements * sizeof(uint32_t) );
    
    for (int i = 0; i < numElements; i++)
        data[i] =
            0xFF000000 | rgbData[i*3] | (rgbData[i*3+1] << 8) | (rgbData[i*3+2] << 16);
    
    return data;
}

Image* createImage(int width, int height)
{
    Image* img = (Image*)malloc(sizeof(Image));
    *img = (Image){.width = width, .height = height, .data = malloc(width*height*sizeof(uint32_t)) };
    memset(img->data, 0, width*height*sizeof(uint32_t));
    return img;
}


Image* loadImage(const char* filename)
{
    int colorType, width, height;
    uint8_t *pixelBuffer;
    read_png_file(filename, &width, &height, &colorType, (uint8_t**)&pixelBuffer );
    
        
    uint32_t *data = (uint32_t*)pixelBuffer;
    if (colorType == PNG_COLOR_TYPE_RGB)
    {
        data = convertRgbToRgba(pixelBuffer, width * height);
        free(pixelBuffer);
    } 
    
    Image *res = (Image*)malloc(sizeof(Image));
    *res = (Image){.width = width, .height = height, .data = data};
    return res;
}

/*
Image* loadImageFromMemory(const uint8_t *const pngData, int pngDataSize)
{
    int colorType;
    Image *res = (Image*)malloc(sizeof(Image));
    read_png_from_memory(pngData, pngDataSize, &res->width, &res->height, &colorType, (uint8_t**)&res->data );

    if (colorType == PNG_COLOR_TYPE_RGB)
    {
        uint32_t *data = convertRgbToRgba( (uint8_t*)res->data, res->width * res->height);
        free(res->data);
        res->data = data;
    } 
    
    return res;
}
*/

Image* cloneImage(const Image * const img)
{
    Image *res = (Image*)malloc(sizeof(Image));
    *res = (Image){ .width = img->width, .height= img->height, 
                    .data = (uint32_t*)malloc( img->width * img->height * sizeof(uint32_t))};
                    
    memcpy(res->data, img->data, img->width * img->height * sizeof(uint32_t));
    return res;

}

int getImageWidth(Image* img) { return img->width;}

int getImageHeight(Image* img) {return img->height;}

