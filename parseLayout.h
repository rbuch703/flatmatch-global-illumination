
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#include <vector>
#include "rectangle.h"
#include <ostream>

using namespace std;

Geometry parseLayout(const char* const filename, const float scaling);
void writeJsonOutput(Geometry geo, ostream &jsonGeometry);

extern "C" {
char* getJsonFromLayout(const char* const filename,float scaling);
}

#endif

