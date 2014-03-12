
#include <list>
#include "parseLayout.h"

#include <assert.h>

void read_png_file(const char* file_name, int &width, int& height, int& color_type, uint8_t* &pixel_buffer );
void write_png_file(const char* file_name, int width, int height, int color_type, uint8_t *pixel_buffer);
static const int PNG_COLOR_TYPE_RGB  = 2;
static const int PNG_COLOR_TYPE_RGBA = 6;


static const double HEIGHT = 200;


using namespace std;

list<Rectangle> parseLayout(const char* const filename) {

    list<Rectangle> segments;
    int width, height, color_type;
    uint32_t *pixel_buffer;
    read_png_file(filename, width, height, color_type, (uint8_t*&)pixel_buffer);
    cout << "read image of size " << width << "x" << height << " with color_type " << color_type << endl;
    assert (color_type == PNG_COLOR_TYPE_RGBA);
    
    //segments.push_back(WallSegment( 0, 0, 2, 2));
    static const uint32_t BLACK = 0xFF000000;
    //static const uint32_t WHITE = 0xFFFFFFFF;

    for (int y = 1; y < height; y++)
        for (int x = 1; x < width; x++) {
            if (pixel_buffer[(y-1) * width + (x)] != BLACK && pixel_buffer[y * width + x] == BLACK)
            {
                int start_x = x;
                while (x < width && 
                       pixel_buffer[(y-1) * width + x] != BLACK && 
                       pixel_buffer[ y    * width + x] == BLACK)
                    x++;
                segments.push_back(Rectangle( Vector3(x,y,0), Vector3(start_x-x,0,0), Vector3(0,0,HEIGHT)));        
//                segments.push_back(WallSegment( x, y, start_x, y));
            }
            
            if (pixel_buffer[(y-1) * width + (x)] == BLACK && pixel_buffer[y * width + x] != BLACK)
            {
                int start_x = x;
                while (x < width && 
                       pixel_buffer[(y-1) * width + x] == BLACK && 
                       pixel_buffer[ y    * width + x] != BLACK)
                    x++;
                    
                segments.push_back(Rectangle( Vector3(start_x,y,0), Vector3(x-start_x,0,0), Vector3(0,0,HEIGHT)));        
                //segments.push_back(WallSegment( start_x, y, x, y));
            }
        }


    for (int x = 1; x < width; x++) 
        for (int y = 1; y < height; y++) {
            if (pixel_buffer[y * width + (x-1)] != BLACK && pixel_buffer[y * width + x] == BLACK)
            {
                int start_y = y;
                while (y < height && 
                       pixel_buffer[y * width + (x-1)] != BLACK && 
                       pixel_buffer[y * width +  x   ] == BLACK)
                    y++;
                
                segments.push_back(Rectangle( Vector3(x,start_y,0), Vector3(0,y-start_y,0), Vector3(0,0,HEIGHT)));
                //segments.push_back(WallSegment( x, start_y , x, y));
            }
            
            if (pixel_buffer[y * width + (x-1)] == BLACK && pixel_buffer[y * width + x] != BLACK)
            {
                int start_y = y;
                while (y < height && 
                       pixel_buffer[y * width + (x-1)] == BLACK && 
                       pixel_buffer[y * width +  x   ] != BLACK)
                    y++;
                
                segments.push_back(Rectangle( Vector3(x,y,0), Vector3(0,start_y-y,0), Vector3(0,0,HEIGHT)));
                //segments.push_back(WallSegment( x, y, x, start_y));
            }
        
        }   
     
    delete [] pixel_buffer;
    return segments;
}

