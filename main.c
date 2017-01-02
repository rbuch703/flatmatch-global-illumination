
#include <stdint.h>
#include <stdio.h> //for snprintf
#include <string.h> //for memset
#include <stdlib.h>

#include "rectangle.h"
#include "geometry.h"
#include "parseLayout.h"
#include "global_illumination_cl.h"
#include "global_illumination_native.h"
#include "radiosityNative.h"

typedef enum MODE {PHOTON_NATIVE, PHOTON_CL, AMBIENT_OCCLUSION, RADIOSITY_NATIVE} MODE;


int main(int argc, const char** argv)
{

    if (argc < 2 || argc > 3)
    {
        printf("usage: %s <layout image> [<scale>]\n\n", argv[0]);
        printf("  - The <layout image> is the name of an existing 'png' image file.\n");
        printf("  - The optional <scale> is a floating point number in giving the image\n");
        printf("    scale in pixels/m. If none is given a default value of 30.0 is assumed.\n\n");
        return -1;
    }
    
    MODE illuminationMode = PHOTON_CL;
  
    //string filename = (argc >= 2) ? argv[1] : "out.png" ;
    float scale = argc < 3 ? 30 : atof(argv[2]);
    Image* img = loadImage(argv[1]);
    

    char* jsonCollisionMap = buildCollisionMap(img);
    FILE* f = fopen("collisionMap.json", "wb");
    fwrite( jsonCollisionMap, strlen(jsonCollisionMap), 1, f);
    fclose(f);
    free(jsonCollisionMap);

    /** scale is passed in the more human-readable pixel/m, 
        but the geometry loader needs it in m/pixel */
    const float TILE_SIZE = 200;   //lightmap texels per m²
    Geometry *geo = parseLayout(img, 1/scale, TILE_SIZE);
    freeImage(img);

    char* s = getJsonString(geo);
    /*FILE**/ f = fopen("geometry.json", "wb");
    fwrite(s, strlen(s), 1, f); //don't write zero-termination
    fclose(f);
    free(s);


    printf("[INF] Layout consists of %d walls (%.2fk texels) %d windows, %d lights\n", geo->numWalls, geo->numTexels/1000.0, geo->numWindows, geo->numLights);
    

    int numSamplesPerArea = 1000 * 1000 * 100;   // rays per square meter of window/light surface
    
    switch (illuminationMode)
    {
        case PHOTON_NATIVE: performPhotonMappingNative(geo, numSamplesPerArea); break;
        case PHOTON_CL:    performGlobalIlluminationCl(geo, numSamplesPerArea); break;
        case AMBIENT_OCCLUSION: performAmbientOcclusionNative(geo );            break;
        case RADIOSITY_NATIVE: performRadiosityNative(geo); break;
    }
    
    if (illuminationMode == PHOTON_NATIVE || illuminationMode == PHOTON_CL)
    {
        for ( int i = 0; i < geo->numWalls; i++)
        {
            Rectangle *obj = &geo->walls[i];
            float tilesPerSample = getNumTiles(obj) / ( getArea(obj) * numSamplesPerArea);  
            int baseIdx = obj->lightmapSetup.s[0];

            for (int j = 0; j < getNumTiles(obj); j++)
                geo->texels[baseIdx + j] = mul(geo->texels[baseIdx +j], 0.35 * tilesPerSample);
        }    
    }

    //write texture files
    char *filename;
    for ( int i = 0; i < geo->numWalls; i++)
    {
        int numChars = snprintf(filename, 0, "tiles/tile_%d.png", i);
        filename = (char*) malloc (numChars+1); //plus zero-termination
        snprintf(filename, numChars+1, "tiles/tile_%d.png", i);
        saveAs(    &geo->walls[i], filename, geo->texels,
            illuminationMode == AMBIENT_OCCLUSION || 
            illuminationMode == PHOTON_NATIVE ||
            illuminationMode == RADIOSITY_NATIVE);
            
        free(filename);
        //saveAsRaw( &geo.walls[i], (filename + ".raw").c_str(), geo.texels);
    }
    

    freeGeometry(geo);

    return 0;
}
