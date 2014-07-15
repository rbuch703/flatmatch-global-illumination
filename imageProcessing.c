
#include "imageProcessing.h"
#include "rectangle.h" // for SUPER_SAMPLING
#include <string.h> //for memcpy()
#include <math.h>   // for exp()


static uint8_t clamp(float d)
{
    if (d < 0) d = 0;
    if (d > 255) d = 255;
    return d;
}

//convert light energy to perceived brightness
float convert(float color)
{
    return 1 - exp(-color);
}

int isBlack(const uint8_t *data, int width, int height, int x, int y)
{
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height-1;
    
    int idx = (y * width + x ) * 3;
    return (data[idx] == 0 && data[idx+1] == 0 && data[idx+2] == 0);
}

typedef struct Pixel {
    int r,g,b;
} Pixel;

Pixel getPixelData(const uint8_t *data, int width, int height, int x, int y)
{
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height-1;
    
    int idx = (y * width + x ) * 3;
    Pixel res = { .r = data[idx], .g = data[idx+1], .b = data[idx+2] };
    return res;
}

void mark(uint8_t *data, int width, int height, int x, int y, int r, int g, int b)
{
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height-1;
    
    int idx = (y*width + x)*3;
    data[idx  ] = r;
    data[idx+1] = g;
    data[idx+2] = b;
}

int hasBlackNeighbor(uint8_t *data, int width, int height, int x, int y)
{
    for (int dx = x-1; dx <= x+1; dx++)
        for (int dy = y-1; dy <= y+1; dy++)
            if (isBlack(data, width, height, dx, dy))
                return 1;
                
    return 0;
}

int hasNonBlackNeighbor(uint8_t *data, int width, int height, int x, int y)
{
    for (int dx = x-1; dx <= x+1; dx++)
        for (int dy = y-1; dy <= y+1; dy++)
            if (!isBlack(data, width, height, dx, dy))
                return 1;
                
    return 0;
}

void selectiveDilate(uint8_t *data, int width, int height)
{
    uint8_t *dataTmp = malloc(width*height*3);
    memcpy(dataTmp, data, width*height*3);
    //memset(dataTmp, 0, width*height*3);
    //int x = 10;
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
        {
            /*
            if ( (x+y) % 2 == 0)
            {
                printf("(%d, %d)\n", x, y);
                mark(data, width, height, x, y, 0, 255, 0);
            }*/
            /*if (isBlack(data, width, height, x, y))
                mark(data, width, height, x, y, 0, 255, 0);
            else */if (hasBlackNeighbor(data, width, height, x, y) &&
                       hasNonBlackNeighbor(data, width, height, x, y))
            {
                //int neighbors[9] = {0,0,0, 0,0,0, 0,0,0};
                Pixel brightest = {.r = 0, .g = 0, .b = 0};
                
                for (int dx = -1; dx <= 1; dx++)
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        Pixel p = getPixelData(data, width, height, x+dx, y+dy);
                        if ( p.r + p.g + p.b > brightest.r + brightest.g + brightest.b)
                            brightest = p;
                    }
                    
                dataTmp[ (y * width + x) * 3 + 0] = brightest.r;
                dataTmp[ (y * width + x) * 3 + 1] = brightest.g;
                dataTmp[ (y * width + x) * 3 + 2] = brightest.b;
                    
                //int numNeighbors = 0;
                
                //mark(dataTmp, width, height, x, y, 0, 0, 255);
            }
        }
        
    memcpy(data, dataTmp, width*height*3);
    free(dataTmp);
}

void subsampleAndConvertToPerceptive(Vector3* lights, uint8_t *dataOut, int width, int height)
{
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
    {
        float sum_r = 0;
        float sum_g = 0;
        float sum_b = 0;
        float count = 0;
        
        for (int dx = 0; dx < SUPER_SAMPLING; dx++)
            for (int dy = 0; dy < SUPER_SAMPLING; dy++)
        {
            float r = lights[(y*SUPER_SAMPLING+dy)* (width * SUPER_SAMPLING) + (x*SUPER_SAMPLING+dx)].s[0];
            float g = lights[(y*SUPER_SAMPLING+dy)* (width * SUPER_SAMPLING) + (x*SUPER_SAMPLING+dx)].s[1];
            float b = lights[(y*SUPER_SAMPLING+dy)* (width * SUPER_SAMPLING) + (x*SUPER_SAMPLING+dx)].s[2];
            if (r != 0.0f && g != 0.0f && b != 0.0f)
            {
                sum_r += r;
                sum_g += g;
                sum_b += b;
                count +=1;
            }
        }

        dataOut[(y * width + x) * 3 + 0] = clamp( convert(sum_r / count ) * 255);
        dataOut[(y * width + x) * 3 + 1] = clamp( convert(sum_g / count ) * 255);
        dataOut[(y * width + x) * 3 + 2] = clamp( convert(sum_b / count ) * 255);
            
    }

}

