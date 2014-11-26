
#ifndef PARSELAYOUT_H
#define PARSELAYOUT_H

#include "rectangle.h"
//#include <ostream>

//using namespace std;

//void writeJsonOutputStream(Geometry geo, ostream &jsonGeometry);

#ifdef __cplusplus
extern "C" {
#endif

void writeJsonOutput(Geometry geo, const char*filename);

Geometry parseLayout(const char* const filename, const float scaling);
Geometry parseLayoutMem(const uint8_t *data, int dataSize, const float scaling);

/* these methods exist mostly for the emscripten interface. The returned pointer 
 * refers to a static object without parseLayoutStatic (hence the method's name).
 * The caller does not own the resulting Geometry object and thus does not have 
 * to free it */
Geometry* parseLayoutStatic(const char* filename, float scale);
Geometry* parseLayoutStaticMem(const uint8_t* data, int dataSize, float scale);

char* getJsonFromLayout(const char* const filename,float scaling);
char* getJsonFromLayoutMem( const uint8_t *data, int dataSize, float scaling);

#ifdef __cplusplus
}
#endif


#endif

