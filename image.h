#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include "png_helper.h"

class Image {

public:
    Image(const Image &other): Image(other.width, other.height, other.data)
    {
    }
    
    Image(int width, int height): width(width), height(height)
    {
        this->data = new uint32_t[width*height];
        memset(this->data, 0, sizeof(uint32_t) * width * height);        
    }


    Image(int width, int height, uint32_t *data): width(width), height(height)
    {
        this->data = new uint32_t[width*height];
        memcpy(this->data, data, sizeof(uint32_t) * width*height);
    }
    
    ~Image()
    {
        delete [] data;
    }
    
    
    uint32_t get(int x, int y) const
    {
        x = max( min(x, width-1), 0);
        y = max( min(y, height-1), 0);
        return data[y*width+x];
    }

    bool set(int x, int y, uint32_t val)
    {
        if ( x < 0) return false;
        if ( x >= width) return false;
        if ( y < 0) return false;
        if ( y >= height) return false;
        
        data[y*width+x] = val;
        return true;
    }

    unsigned int distanceTransform()
    {

        vector<pair<int, int>> openList;
        unsigned int distance = 1;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
        {
            if ( this->get(x,y) == distance)
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
                            
                        if ( this->get(x, y) == 0)
                        {
                            this->set(x, y, distance+1);
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

    void floodFill(int x, int y, uint32_t value, uint32_t background)
    {
        if (x < 0 || x >= width) return;
        if (y < 0 || y >= height) return;
    
        if (this->get(x, y) != background)
            return;
            
        this->set(x, y, value);
        
        this->floodFill( x-1, y-1, value, background);
        this->floodFill( x-1, y  , value, background);
        this->floodFill( x-1, y+1, value, background);

        this->floodFill( x  , y-1, value, background);
    //  this->floodFill( x  , y  , value, background);
        this->floodFill( x  , y+1, value, background);

        this->floodFill( x+1, y-1, value, background);
        this->floodFill( x+1, y  , value, background);
        this->floodFill( x+1, y+1, value, background);
    }

    void saveAs(const char* filename) const
    {
        write_png_file( filename, width, height, PNG_COLOR_TYPE_RGBA, (uint8_t*)data);
    
    }
    
    int getHeight() const { return height;}
    int getWidth()  const { return width;}
    
private:
    int width, height;
    uint32_t *data;

};

#endif

