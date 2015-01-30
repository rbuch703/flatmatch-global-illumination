#ifndef GLOBAL_ILLUMINATION_NATIVE
#define GLOBAL_ILLUMINATION_NATIVE

#ifdef __cplusplus
extern "C" {
#endif

#include "rectangle.h"
#include "geometry.h"

struct BspTreeNode;
//typedef struct BspTreeNode BspTreeNode;

void performPhotonMappingNative(Geometry *geo, int numSamplesPerArea);
void performRadiosityNative(Geometry *geo);

void performAmbientOcclusionNative(Geometry *geo);
/* The following three functions are declared here only for the JavaScript 
   interface. There, they allow to build the BSP tree once, and then use it to
   compute the ambient occlusion individually for each wall, allowing to 
   execute JavaScript code (here: uploading the ambient occlusion as a texture
   in between. These methods are considered internal for the C/C++ interface.
   */
void performAmbientOcclusionNativeOnWall(Geometry* geo, const struct BspTreeNode *root, Rectangle* wall);
struct BspTreeNode* buildBspTree( Rectangle* items, int numItems);  
void freeBspTree(struct BspTreeNode *root);



#ifdef __cplusplus
}
#endif

#endif
