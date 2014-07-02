
#include <vector>
#include <list>
#include "parseLayout.h"

#include <assert.h>
#include <string>
#include "png_helper.h"
#include <iostream>
static const double HEIGHT = 200;


using namespace std;

string getColorName(uint32_t col)
{
    switch (col)
    {
        case 0xFF000000: return "black";
        case 0xFF808080: return "gray";
        case 0xFFFFFFFF: return "white";
        case 0xFF00FF00: return "green";
        default: return "(unknown)";
    }
   
}


//#error continuehere: add scaling factor, scale height, use real height values
vector<Rectangle> parseLayout(const char* const filename, const float ) {
    static const Vector3 wallColor = createVector3(0.8, 0.8, 0.8);
    static const Vector3 windowColor = createVector3(15, 14, 12);
    list<Rectangle> segments;
    int width, height, color_type;
    uint32_t *pixel_buffer;
    read_png_file(filename, &width, &height, &color_type, (uint8_t**)&pixel_buffer );
    cout << "read image of size " << width << "x" << height << " with color_type " << color_type << endl;
    assert (color_type == PNG_COLOR_TYPE_RGBA);
    
    //segments.push_back(WallSegment( 0, 0, 2, 2));
    static const uint32_t BLACK = 0xFF000000;
    static const uint32_t WHITE = 0xFFFFFFFF;
    //static const uint32_t GRAY =  0xFF808080;
    static const uint32_t GREEN = 0xFF00FF00;
    
    static const float HEIGHT = 2.50 * 100;
    static const float WINDOW_LOW = 0.90 * 100;
    static const float WINDOW_HIGH= 2.20 * 100;
    static const float WINDOW_HEIGHT = WINDOW_HIGH - WINDOW_LOW;
    
    for (int y = 1; y < height; y++)
    {
        for (int x = 1; x < width;) {
            uint32_t pxAbove = pixel_buffer[(y-1) * width + (x)];
            uint32_t pxHere =  pixel_buffer[(y  ) * width + (x)];
            if (pxAbove == pxHere)
            {
                x++;
                continue;
            }
                
            int startX = x;
            
            while (x < width && (pxAbove == pixel_buffer[(y-1) * width + (x)]) && (pxHere == pixel_buffer[(y) * width + (x)]))
                x++;
                
            int endX = x;//-1;
            //cout << "found edge at (" << startX << ", " << y << ")  with length " << (endX-startX) << " and transition " << getColorName(pxAbove) << "/" << getColorName(pxHere) << endl;
            
            //if (pxAbov
            
            if (pxAbove == BLACK && pxHere == WHITE) //transition from wall to inside area
                segments.push_back(createRectangleWithColor( createVector3(startX,y,0),  
                                                             createVector3(endX - startX,0,0),
                                                             createVector3(0,0,HEIGHT), wallColor));
            else if (pxAbove == WHITE && pxHere == BLACK) // transition from inside area to wall
                segments.push_back(createRectangleWithColor( createVector3(endX,y,0),  
                                                             createVector3(startX - endX,0,0), 
                                                             createVector3(0,0,HEIGHT), wallColor));
            else if (pxAbove == GREEN && pxHere == WHITE) //transition from window to inside area
            {
                segments.push_back(createRectangleWithColor( createVector3(startX,y,0),  
                                                             createVector3(endX - startX,0,0), 
                                                             createVector3(0, 0, WINDOW_LOW), wallColor ));
                segments.push_back(createRectangleWithColor( createVector3(startX,y,WINDOW_LOW), 
                                                             createVector3(endX - startX,0,0), 
                                                             createVector3(0, 0, WINDOW_HEIGHT), windowColor));
                segments.push_back(createRectangleWithColor( createVector3(startX,y,WINDOW_HIGH), 
                                                             createVector3(endX - startX,0,0), 
                                                             createVector3(0, 0, HEIGHT - WINDOW_HIGH), wallColor));
            }
            else if (pxAbove == WHITE && pxHere == GREEN)
            {
                segments.push_back(createRectangleWithColor( createVector3(endX,y,0),  
                                                             createVector3(startX - endX,0,0), 
                                                             createVector3(0,0,WINDOW_LOW), wallColor ));
                segments.push_back(createRectangleWithColor( createVector3(endX,y,WINDOW_LOW), 
                                                             createVector3(startX - endX,0,0),
                                                             createVector3(0,0,WINDOW_HEIGHT), windowColor));
                segments.push_back(createRectangleWithColor( createVector3(endX,y,WINDOW_HIGH),
                                                             createVector3(startX - endX,0,0), 
                                                             createVector3(0,0,HEIGHT-WINDOW_HIGH), wallColor));
            }
            else
            {    
                //cout << "\t ignoring transition" << endl;
            }
            
        }
    }
    //cout << "  == End of horizontal scan, beginning vertical scan ==" << endl;

    for (int x = 1; x < width; x++)
    {
        for (int y = 1; y < height; ) {
            uint32_t pxLeft = pixel_buffer[y * width + (x - 1) ];
            uint32_t pxHere = pixel_buffer[y * width + (x    ) ];
            if (pxLeft == pxHere)
            {
                y++;
                continue;
            }
                
            int startY = y;
            
            while (y < height && (pxLeft == pixel_buffer[y * width + (x-1)]) && (pxHere == pixel_buffer[y * width + x]))
                y++;
                
            int endY = y;//-1;
            //cout << "found edge (" << x << ", " << startY << ") - (" << x << ", " << endY << ") with " << getColorName(pxLeft) << "/" << getColorName(pxHere) << endl;
            
            if (pxLeft == BLACK && pxHere == WHITE) //transition from wall to inside area
                segments.push_back(createRectangleWithColor( createVector3(x,endY,0), 
                                                             createVector3(0,startY - endY,0), 
                                                             createVector3(0,0,HEIGHT), wallColor));
            else if (pxLeft == WHITE && pxHere == BLACK) // transition from inside area to wall
                segments.push_back(createRectangleWithColor( createVector3(x,startY,0), 
                                                             createVector3(0,endY - startY,0), 
                                                             createVector3(0,0,HEIGHT), wallColor));
            else if (pxLeft == GREEN && pxHere == WHITE) //transition from window to inside area
            {
                segments.push_back(createRectangleWithColor( createVector3(x,endY,0),  
                                                             createVector3(0, startY - endY,0), 
                                                             createVector3(0,0,WINDOW_LOW), wallColor ));
                segments.push_back(createRectangleWithColor( createVector3(x,endY,WINDOW_LOW), 
                                                             createVector3(0, startY - endY,0), 
                                                             createVector3(0,0,WINDOW_HEIGHT), windowColor));
                segments.push_back(createRectangleWithColor( createVector3(x,endY,WINDOW_HIGH),
                                                             createVector3(0, startY - endY,0), 
                                                             createVector3(0,0,HEIGHT-WINDOW_HIGH), wallColor));
            }
            else if (pxLeft == WHITE && pxHere == GREEN)
            {
                segments.push_back(createRectangleWithColor( createVector3(x,startY,0),  
                                                             createVector3(0, endY - startY,0), 
                                                             createVector3(0,0,WINDOW_LOW), wallColor ));
                segments.push_back(createRectangleWithColor( createVector3(x,startY,WINDOW_LOW), 
                                                             createVector3(0, endY - startY,0), 
                                                             createVector3(0,0,WINDOW_HEIGHT), windowColor));
                segments.push_back(createRectangleWithColor( createVector3(x,startY,WINDOW_HIGH),
                                                             createVector3(0, endY - startY,0), 
                                                             createVector3(0,0,HEIGHT-WINDOW_HIGH), wallColor));
            } 
            else
            {
                //cout << "\t ignoring transition" << endl;
            }
            
        }
    }

    free( pixel_buffer);

    //find AABB of apartment, as this is a conservative estimate for the floor and ceiling extents
    float min_x = segments.begin()->pos.s[0];
    float max_x = segments.begin()->pos.s[0];
    float min_y = segments.begin()->pos.s[1];
    float max_y = segments.begin()->pos.s[1];
    for (list<Rectangle>::const_iterator seg = segments.begin(); seg != segments.end(); seg++)
    {
        max_x = max(max_x, seg->pos.s[0]);
        min_x = min(min_x, seg->pos.s[0]);
        max_y = max(max_y, seg->pos.s[1]);
        min_y = min(min_y, seg->pos.s[1]);
        
        float x = seg->pos.s[0] + seg->width.s[0]; //width may be negative, so pos+width can
        float y = seg->pos.s[1] + seg->width.s[1]; //be smaller or larger than pos alone

        max_x = max(max_x, x);
        min_x = min(min_x, x);
        max_y = max(max_y, y);
        min_y = min(min_y, y);
        
    }

    cout << "floor/ceiling size is: (" << min_x << ", " << min_y << "), " << (max_x - min_x) << "x" << (max_y - min_y) << endl;

    
    segments.push_front( createRectangleWithColor( createVector3(min_x,min_y,HEIGHT), 
                                                   createVector3(max_x - min_x, 0, 0), 
                                                   createVector3(0, max_y - min_y, 0), wallColor));  // ceiling
    segments.push_front( createRectangleWithColor( createVector3(min_x,min_y,0), 
                                                   createVector3(0, max_y-min_y, 0), 
                                                   createVector3(max_x-min_x, 0, 0), wallColor));    // floor
    
    cout << "registered " << segments.size() << " wall segments" << endl;
    return vector<Rectangle>(segments.begin(), segments.end());
}

