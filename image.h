#ifndef IMAGE_H
#define IMAGE_H

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

void freeImage(Image *img);
uint32_t getImagePixel(Image* img, int x, int y);
int setImagePixel(Image* img, int x, int y, uint32_t val);
unsigned int distanceTransform(Image *img);
void floodFillImage(Image *img, int x, int y, uint32_t value, uint32_t background);
void saveImageAs(Image *img, const char* filename);

#endif

