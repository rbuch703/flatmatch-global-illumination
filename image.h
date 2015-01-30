#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    int x,y;
} Point2D;



typedef struct {
    Point2D *data;
    unsigned int numItems;
    unsigned int maxNumItems;
} Point2DArray;

Point2DArray initPoint2DArray();
void insertIntoPoint2DArray(Point2DArray *arr, int x, int y);
void freePoint2DArray(Point2DArray *arr);


typedef struct {
    int width, height;
    uint32_t *data;
} Image;

void freeImage(Image *img); //frees the image data, as well as the image pointer itself (not safe for stack allocation)
uint32_t getImagePixel(const Image* const img, int x, int y);
int setImagePixel(Image* img, int x, int y, uint32_t val);
unsigned int distanceTransform(Image *img);
void floodFillImage(Image *img, int x, int y, uint32_t value, uint32_t background);
void saveImageAs(Image *img, const char* filename);
int getImageWidth(Image* img);
int getImageHeight(Image* img);

Image* createImage(int width, int height);
Image* cloneImage(const Image * const img);
Image* loadImage(const char* filename);
Image* loadImageFromMemory(const uint8_t *const pngData, int pngDataSize);

#ifdef __cplusplus
}
#endif


#endif

