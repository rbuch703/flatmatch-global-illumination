
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#include <vector>
#include "rectangle.h"
#include <ostream>

using namespace std;

void writeJsonOutput(Geometry geo, ostream &jsonGeometry);

extern "C" {
int parseLayout(const char* const filename, const float scaling, Geometry* geo);
int parseLayoutMem(const uint8_t *data, int dataSize, const float scaling, Geometry* geo);

char* getJsonFromLayout(const char* const filename,float scaling);
char* getJsonFromLayoutMem( const uint8_t *data, int dataSize, float scaling);

}

#endif

