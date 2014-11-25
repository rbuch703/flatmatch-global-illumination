
#include "rectangle.h"
#include "parseLayout.h"
//#include "png_helper.h"
#include "vector3_cl.h"

#include <list>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdint.h>
//#include <string>
#include <string.h> //for memset


#include "global_illumination_cl.h"
#include "global_illumination_native.h"
using namespace std;


//Rectangle* walls = NULL;
//cl_int numWalls;

//vector<Rectangle> windows, lights, box;

Vector3* lightColors = NULL;
cl_int numTexels;
//int width, height;
//pair<float, float> startingPos;

void freeGeometry(Geometry geo)
{
    free(geo.walls);
    free(geo.boxWalls);
    free(geo.lights);
    free(geo.windows);
}


Geometry loadGeometry(string filename, float scale)
{
    Geometry geo;
    numTexels = parseLayout(filename.c_str(), scale, &geo);

    cout << "[DBG] allocating " << (numTexels * sizeof(Vector3)/1000000) << "MB for texels" << endl;
    if (numTexels * sizeof(Vector3) > 1000*1000*1000)
    {
        cout << "[Err] Refusing to allocate more than 1GB for texels, this would crash most GPUs. Exiting ..." << endl;
    }

    if (0 != posix_memalign( (void**) &lightColors, 16, numTexels * sizeof(Vector3)))
    {
        cout << "[Err] aligned memory allocation failed, exiting ..." << endl;
        exit(0);
    }
    
    for (int i = 0; i < numTexels; i++)
        lightColors[i] = createVector3(0,0,0);
        
    return geo;
}

enum MODE {PHOTON_NATIVE, PHOTON_CL, AMBIENT_OCCLUSION};

int main(int argc, const char** argv)
{
    //cout << "Rectangle size is " << sizeof(Rectangle) << " bytes" << endl;

    if (argc < 2 || argc > 3)
    {
        cout << "usage: " << argv[0] << " <layout image> [<scale>]" << endl;
        cout << endl;
        cout << "  - The <layout image> is the name of an existing 'png' image file." << endl;
        cout << "  - The optional <scale> is a floating point number in giving the image" << endl;
        cout << "    scale in pixels/m. If none is given a default value of 30.0 is assumed." << endl;
        cout << endl;
        exit(0);
    }
    
    MODE illuminationMode = PHOTON_NATIVE;//AMBIENT_OCCLUSION;
    
    
    //string filename = (argc >= 2) ? argv[1] : "out.png" ;
    

    float scale = argc < 3 ? 30 : atof(argv[2]);
    
    //scale is passed in the more human-readable pixel/m, but the geometry loader needs it in m/pixel
    Geometry geo = loadGeometry(argv[1], 1/scale);

    /*vector<Rectangle> windows;
    for ( int i = 0; i < numWalls; i++)
    {
        if (objects[i].pos.s[2] == 0.90 * 100)  //HACK: select windows, which start at 90cm height
            windows.push_back( objects[i]);
    }*/
    cout << "[INF] Layout consists of " << geo.numWalls << " walls (" << numTexels/1000000.0 << "M texels) and " << geo.numWindows << " windows" << endl;
    //cout << "total of " << numLightColors << " individual light texels" << endl;



    int numSamplesPerArea = 1000000 * 1;   // rays per square meter of window/light surface
    
    switch (illuminationMode)
    {
        case PHOTON_NATIVE:
            performPhotonMappingNative(&geo, lightColors, numSamplesPerArea);
            break;
            
        case PHOTON_CL:
            performGlobalIlluminationCl(geo, lightColors, numTexels, numSamplesPerArea);
            break;
        case AMBIENT_OCCLUSION:
            performAmbientOcclusionNative(&geo, lightColors);
            break;
    }
    
    
    if (illuminationMode == PHOTON_NATIVE || illuminationMode == PHOTON_CL)
        for ( int i = 0; i < geo.numWalls; i++)
        {
            Rectangle &obj = geo.walls[i];
            float tilesPerSample = getNumTiles(&obj) / ( getArea(&obj) * numSamplesPerArea);  
            int baseIdx = obj.lightmapSetup.s[0];

            for (int j = 0; j < getNumTiles(&obj); j++)
                lightColors[baseIdx + j] = mul(lightColors[baseIdx +j], 0.35 * tilesPerSample);
        }    

    //write texture files
    char num[50];
    for ( int i = 0; i < geo.numWalls; i++)
    {
        snprintf(num, 49, "%d", i);
        string filename = string("tiles/tile_") + num;
        saveAs(    &geo.walls[i], (filename + ".png").c_str(), lightColors);
        saveAsRaw( &geo.walls[i], (filename + ".raw").c_str(), lightColors);
    }

    ofstream jsonGeometry("geometry.json");
    writeJsonOutput(geo, jsonGeometry);
    jsonGeometry.close();
   
    freeGeometry(geo);
    free( lightColors);


    return 0;
}
