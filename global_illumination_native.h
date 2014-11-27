#ifndef GLOBAL_ILLUMINATION_NATIVE
#define GLOBAL_ILLUMINATION_NATIVE

#ifdef __cplusplus
extern "C" {
#endif

#include "rectangle.h"

struct BspTreeNode;
//typedef struct BspTreeNode BspTreeNode;

void performPhotonMappingNative(Geometry geo, int numSamplesPerArea);
void performAmbientOcclusionNative(Geometry geo);
void performAmbientOcclusionNativeOnWall(Geometry* geo, const struct BspTreeNode *root, Rectangle* wall);
struct BspTreeNode* buildBspTree( Rectangle* items, int numItems);
void freeBspTree(struct BspTreeNode *root);



#ifdef __cplusplus
}
#endif

#endif
