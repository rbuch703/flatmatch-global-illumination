
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#include <vector>
#include "rectangle.h"

using namespace std;

void parseLayout(const char* const filename, const float scaling, vector<Rectangle> &wallsOut, 
                 vector<Rectangle> &windowsOut, vector<Rectangle> &lightsOut, vector<Rectangle> &boxOut, pair<float, float> &startingPositionOut);


#endif

