
#include <math.h>

#include "vector3_cl.h"
#include "rectangle.h"

#include <stdio.h> //for printf()
#include <string.h> //for memcpy()
#include <stdlib.h>

#include "geoSphere.h"
#include "global_illumination_native.h"
//#include <math.h> //for max()

//Vector3 getDiffuseSkyRandomRay(const Vector3 ndir/*, const Vector3 udir, const Vector3 vdir*/);
//Vector3 getCosineDistributedRandomRay(const Vector3 ndir);
//int getTileIdAt(const Rectangle *rect, const Vector3 p);
//float intersects( const Rectangle *rect, const Vector3 ray_src, const Vector3 ray_dir, const float closestDist);


typedef struct BspTreeNode {
    Rectangle plane;
    Rectangle *items;
    int       numItems;
    struct BspTreeNode *left;
    struct BspTreeNode *right;

} BspTreeNode;


static Vector3 getDiffuseSkyRandomRay(const Vector3 ndir/*, const Vector3 udir, const Vector3 vdir*/)
{
    //HACK: computes a lambertian quarter-sphere (lower half of hemisphere)
    
    // Step 1:Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand()/(double)RAND_MAX);
    float phi = 2 * 3.141592f * (rand()/(double)RAND_MAX);

    // Step 2: Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    if (u < 0)  //project to lower quadsphere (no light from below the horizon)
        u = -u;

    Vector3 udir = initVector3(0,0,1);
    if (fabs( dot(udir, ndir)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        udir = initVector3(0,1,0);

    Vector3 vdir = normalized( cross(udir,ndir));
    udir = normalized( cross(vdir,ndir));

    //# Convert to a direction on the hemisphere defined by the normal
    return add3( mul(udir, u), mul(vdir, v), mul(ndir, n));
}

static Vector3 getCosineDistributedRandomRay(const Vector3 ndir) {
    // Step 1:Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand()/(double)RAND_MAX);
    float phi = 2 * 3.141592f * (rand()/(double)RAND_MAX);

    // Step 2: Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    
    Vector3 udir = initVector3(0,0,1);
    if (fabs( dot(udir, ndir)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        udir = initVector3(0,1,0);

    Vector3 vdir = normalized( cross(udir,ndir));
    udir = normalized( cross(vdir,ndir));

    //# Convert to a direction on the hemisphere defined by the normal
    return add3( mul(udir, u), mul(vdir, v), mul(ndir, n));
}

//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void createBase( const Vector3 ndir, Vector3 *c1, Vector3 *c2) {
    *c1 = vec3(0,0,1);
    if (fabs( dot(ndir, *c1)) >= 0.999999f) //are (nearly) colinear --> cannot build coordinate base
        *c1 = vec3(0,1,0);

        
    *c2 = normalized( cross(*c1,ndir));
    *c1 = normalized( cross(*c2,ndir));

    assert( dot(ndir, *c1) < 1E6); //ensure orthogonality
    assert( dot(ndir, *c2) < 1E6);
    assert( dot(*c1, *c2) < 1E6);
    
    assert( fabsf(length(ndir) -1) < 1E6);    //ensure normality
    assert( fabsf(length(*c1) -1) < 1E6);
    assert( fabsf(length(*c2) -1) < 1E6);
    
}

Vector3 transformToOrthoNormalBase( const Vector3 in, const Vector3 b0, const Vector3 b1, const Vector3 b2)
{
    assert( dot(b0, b1) < 1E6); //ensure orthogonality
    assert( dot(b0, b2) < 1E6);
    assert( dot(b1, b2) < 1E6);
    
    assert( fabsf(length(b0) -1) < 1E6);    //ensure normality
    assert( fabsf(length(b1) -1) < 1E6);
    assert( fabsf(length(b2) -1) < 1E6);
    
    Vector3 res = vec3(
        in.s[0] * b0.s[0] + in.s[1] * b1.s[0] + in.s[2] * b2.s[0],
        in.s[0] * b0.s[1] + in.s[1] * b1.s[1] + in.s[2] * b2.s[1],
        in.s[0] * b0.s[2] + in.s[1] * b1.s[2] + in.s[2] * b2.s[2]        
    );

    return res;
}


//#define DEBUG(X) {X;}
#define DEBUG(X) {}

int findClosestIntersection(Vector3 ray_pos, Vector3 ray_dir, const BspTreeNode *node, float *dist, float distShift, Rectangle **targetOut, int depth)
{

    /*char* spaces = (char*)malloc(depth*2+1);
    for (int i = 0; i < depth*2+1; i++)
        spaces[i] = ' ';
    spaces[depth*2] = '\0';*/
    DEBUG (printf ("%sdepth is %d, depthShift %f\n", spaces, depth, distShift));
    if (node->left || node->right)
        DEBUG(printf("%ssplit plane: %d/%d/%d\n",  spaces, node->plane.lightmapSetup.s[0], node->plane.lightmapSetup.s[1], node->plane.lightmapSetup.s[2]));
      
    int hasHit = 0;
    for ( int i = 0; i < node->numItems; i++)
    {
        DEBUG(printf("%stesting local rect %d/%d/%d\n", spaces, node->items[i].lightmapSetup.s[0], node->items[i].lightmapSetup.s[1], node->items[i].lightmapSetup.s[2]));
        float dist_new = intersects(&(node->items[i]), ray_pos, ray_dir, *dist);
        if (dist_new == -1)
        //if (dist_new < 0)
            continue;
        DEBUG(printf("%s#hit at dist=%f\n", spaces, dist_new));
            
        if (dist_new + distShift < *dist) {
            *targetOut = &(node->items[i]);
            *dist = dist_new + distShift;
            hasHit = 1;
        }
    }


    //printf("%sleft  node is 0x%x\n", spaces, node->left);
    //printf("%sright node is 0x%x\n", spaces, node->right);

    
    /** FIXME: if ray_pos is on the left side, and ray_dir looks away from the plane, intersections for the right plane would
                all be behind the ray_pos, and thus are irrelevant; same goes for the other way around
    */
   
    //has no child nodes --> no further geometry to check
    if (!node->left &&  !node->right)
        return hasHit;
    
    
    Vector3 splitPlaneNormal = node->plane.n;
    Vector3 planeToRaySrc = sub(ray_pos, node->plane.pos);
    if ( dot(planeToRaySrc, splitPlaneNormal) < 0)  //ensure that the split plane normal faces towards the ray source
        splitPlaneNormal = neg(splitPlaneNormal);
        
    int facesAwayFromSplitPlane = dot(splitPlaneNormal, ray_dir) >= 0;
    
    
    float pos = getDistanceToPlane( &(node->plane), ray_pos);
    //printf("\tcamera distance to split plane is %f\n", pos);
    
    if (pos < 0)    //we are left --> search left and center first, then right
    {

        int hasChildHit = 0;
        DEBUG(printf("%stesting left child node first\n", spaces));
        if (node->left)
            hasChildHit = findClosestIntersection(ray_pos, ray_dir, node->left, dist, distShift, targetOut, depth+1);
        
        
        /* Possible hits in the 'right' set are guaranteed to be further away that those in the 'left' set.
         * So only test right when there was no hit in left child node: (there is no guarantee as to the
         * relative location of rectangle in the 'center' and 'right' sets, so a hit in the 'center' set
         * does not guarantee that there is no closer 'right' hit). 
         * Also, if the ray faces away from the split plane, it cannot 'see' it, and thus cannot see 
         * anything beyond it. So, in that case, no hit in node->right can occur, and thus does not need
         * to be tested.
         */
        if (!hasChildHit && node->right && !facesAwayFromSplitPlane)
        {
            DEBUG(printf("%salso testing right child node\n", spaces));
            float planeDist = distanceOfIntersectionWithPlane(ray_pos, ray_dir, node->plane.n, node->plane.pos);
            if (planeDist < 0) 
              planeDist = 0;
            ray_pos = add(ray_pos, mul(ray_dir, planeDist));

            hasHit |= findClosestIntersection(ray_pos, ray_dir, node->right, dist, distShift + planeDist, targetOut, depth+1);
        } else
        {
            if (hasChildHit)  DEBUG(printf("%sNOT testing right child, because we already had a hit\n", spaces));
            if (!node->right) DEBUG(printf("%sNOT testing right child, because there is none\n", spaces));
            if (facesAwayFromSplitPlane) DEBUG(printf("%sNOT testing right child, because the ray faces away from it\n", spaces));
        }
        hasHit |= hasChildHit;
    }
    else  //if (pos >= 0)
    {
        DEBUG(printf("%stesting right child node first\n", spaces));
        int hasChildHit = 0;
        if (node->right)
            hasChildHit = findClosestIntersection(ray_pos, ray_dir, node->right, dist, distShift, targetOut, depth+1);

        if (!hasChildHit && node->left && !facesAwayFromSplitPlane)
        {
            DEBUG(printf("%salso testing left child node\n", spaces));
            float planeDist = distanceOfIntersectionWithPlane(ray_pos, ray_dir, node->plane.n, node->plane.pos);
            if (planeDist < 0) 
              planeDist = 0;
            ray_pos = add(ray_pos, mul(ray_dir, planeDist));
            hasHit |= findClosestIntersection(ray_pos, ray_dir, node->left, dist, distShift + planeDist, targetOut, depth+1);
        }
        hasHit |= hasChildHit;
    
    }    
    return hasHit;
}


static void tracePhoton(const Rectangle *window, const BspTreeNode *root, Vector3 *lightColors, const int isWindow)
{

    
    //Vector3 lightColor = 
    Vector3 lightColor = isWindow ? 
        initVector3(18, 18, 18):  //slightly yellow-ish for outside areas
        initVector3(16, 16, 18);  //slightly blue-ish for artificial light

    const int MAX_DEPTH = 8;

    float dx = rand()/(double)RAND_MAX;
    float dy = rand()/(double)RAND_MAX;

    //Vector3 ray_dir = getDiffuseSkyRandomRay(rng_state, window->n);//, normalize(window.width), normalize(window.height));
    Vector3 ray_dir = isWindow ? 
        getDiffuseSkyRandomRay(window->n) :
        getCosineDistributedRandomRay(window->n);
    //move slightly in direction of ray_dir to prevent self-intersection on the light source geometry
    Vector3 pos = add4(
            window->pos, mul(ray_dir, 1E-5f), 
            mul(window->width, dx), mul( window->height,dy) );
    
    for (int depth = 0; depth < MAX_DEPTH; depth++)
    {
        /* WARNING: OpenCL objects in the 'constant' memory area on AMD hardware have their own address space
                    The first object in this address space can have address 0x00000000, so a 'null' pointer 
                    can indeed be valid here --> comparing 'hitObj' to 0 does not return whether hitObj points
                    to a valid object */
        Rectangle* hitObj = 0;
        float dist_out = INFINITY;

        //printf("work_item %d, pos (%f,%f,%f), dir (%f,%f,%f) \n", get_global_id(0), pos.s0, pos.s1, pos.s2, ray_dir.s0, ray_dir.s1, ray_dir.s2);
        
        findClosestIntersection(pos, ray_dir, root, &dist_out, 0, &hitObj, 0);
        
        if (dist_out == INFINITY)
            return;
            
        //if ( hitObj->color.x > 1.0f || hitObj->color.y > 1.0f || hitObj->color.z > 1.0f)
        //    return; //hit a light source;
            
        //hit_obj[get_global_id(0)] = closestObject;
        //re-set the ray position to the intersection point (which is the starting position for the next ray);
        pos = add(pos, mul( ray_dir, dist_out));
        
        int tile_id = getTileIdAt( hitObj, pos);
        int light_idx = hitObj->lightmapSetup.s[0] + tile_id;
        
        /*if (light_idx >= numLightColors)
        {
            printf("invalid light index %d\n", light_idx);
            return;
        }*/
        
        //printf ("lightColor %d/%d is (%f, %f, %f)\n", get_global_id(0), depth, lightColor.x, lightColor.y, lightColor.z);
        
        //pos = hit_pos;
        //Vector3 udir, vdir;
        //createBase(hitObj->n, &udir, &vdir);
        //printf("work_item %d, base1 (%f,%f,%f), base2 (%f,%f,%f) \n", get_global_id(0), udir.s0, udir.s1, udir.s2, vdir.s0, vdir.s1, vdir.s2);
        //printf("work_item %d, hit_pos (%f,%f,%f), new_dir (%f,%f,%f) \n", get_global_id(0), pos.s0, pos.s1, pos.s2, ray_dir.s0, ray_dir.s1, ray_dir.s2);
        
        /* Russian roulette for type of reflection: perfect or diffuse (floor is slightly reflective, everything else is diffuse*/
        if (pos.s[2] < 0.0005 && rand()/(double)RAND_MAX < 0.75)
        {   //perfect reflection
            ray_dir = sub(ray_dir, mul(hitObj->n, 2* (dot(hitObj->n,ray_dir))));
        } else
        {   //diffuse reflection
            ray_dir = getCosineDistributedRandomRay(hitObj->n);

            //hack: make floor slightly brownish
            if (pos.s[2] < 1E-5f)
            {
                //printf("%s\n", "Beep");
                //lightColor.s0 *= 0.0f;
                /*lightColor.s[0] *= 1.0f;
                lightColor.s[1] *= 0.95f;
                lightColor.s[2] *= 0.9f;*/
                lightColor.s[0] *= 1.0f;
                lightColor.s[1] *= 0.85f;
                lightColor.s[2] *= 0.7f;
            }
            lightColor = mul( lightColor, 0.9f);
        }

        //FIXME: make this increment atomic
        lightColors[ light_idx ] = add(lightColors[ light_idx ],  lightColor);

        //ray_dir = getCosineDistributedRandomRay(rng_state, hitObj->n, normalize(hitObj->width), normalize(hitObj->height));
        pos = add(pos, mul(ray_dir, 1E-5f)); //to prevent self-intersection on the light source geometry

            
    }
}



void photonmap( const Rectangle *window, const BspTreeNode* root, Vector3 *lightColors/*, const int numLightColors*/, int isWindow, int numSamples)
{

    //printf("kernel supplied with %d rectangles\n", numRects);
    printf("tracing %s\n", isWindow ? "window" : "light");
    for (int i = 1; i <= numSamples; i++)
    {
        if ( i % 1000000 == 0)
            printf("%dk samples, %.1f %% done.\n", i/1000, (i/(double)numSamples*100));
        tracePhoton(window, root, lightColors, isWindow);
    }
}

/* returns the number of items this subdivision would require to check in the worst case.
   This is used as a measure of the quality of the subdivision
 */
static int getSubdivisionOverhead( BspTreeNode *node, const Rectangle *splitPlane)
{
    if (node->numItems == 0)
        return 0;

    int numLeftItems = 0;
    int numRightItems = 0;
    int numCenterItems = 0;
    for (int i = 0; i < node->numItems; i++)
    {
        Rectangle *rect = &(node->items[i]);
        
        int pos = getPosition( splitPlane, rect);
        numLeftItems  += (pos <  0);
        numRightItems += (pos >  0);
        numCenterItems+= (pos == 0);
    }

    //printf("potential split is %d/%d/%d\n", numLeftItems, numCenterItems, numRightItems);

    return (numLeftItems > numRightItems ? numLeftItems : numRightItems) + numCenterItems;

}

void subdivideNode( BspTreeNode *node, int depth )
{
    /*printf( "subdividing at %f,%f,%f  ---  %f,%f,%f\n", 
        splitPlane->pos.s[0], splitPlane->pos.s[1], splitPlane->pos.s[2],
        splitPlane->n.s[0], splitPlane->n.s[1], splitPlane->n.s[2]);*/
        
    if (node->numItems < 20) return; //is otherwise likely to have a bigger overhead than benefit
    int lowestOverhead = node->numItems;
    int splitPlanePos = 0;

    for (int i = 0; i < node->numItems; i++)
    {
        int overhead = getSubdivisionOverhead(node, &(node->items[i]) );
        //printf("\tOverhead %d is %d\n", i, overhead);
        if (overhead < lowestOverhead)
        {
            lowestOverhead = overhead;
            splitPlanePos = i;
        }
    }
    //printf("optimal split has overhead %d (position %d)\n", lowestOverhead, splitPlanePos);
    
    Rectangle *leftItems = (Rectangle*)malloc(sizeof(Rectangle) * node->numItems);
    Rectangle *rightItems = (Rectangle*)malloc(sizeof(Rectangle) * node->numItems);
    int numLeftItems = 0;
    int numRightItems = 0;
    node->plane = node->items[splitPlanePos];
    /*node->items[splitPlanePos].lightmapSetup.s[0] = 64;
    node->items[splitPlanePos].lightmapSetup.s[1] = 0;
    node->items[splitPlanePos].lightmapSetup.s[2] = 0;*/
    
    for (int i = 0; i < node->numItems; )
    {
        Rectangle *rect = &(node->items[i]);
        
        int pos = getPosition( &(node->plane), rect);
        if (pos < 0)
            leftItems[numLeftItems++] = *rect;
        
        if (pos > 0)
            rightItems[numRightItems++] = *rect;

        if (pos != 0) //smaller or bigger
            node->items[i] = node->items[--node->numItems];
        else
            i++;
    }

    //printf("optimal split is %d/%d/%d\n", numLeftItems, node->numItems, numRightItems);
    
    if (numLeftItems)
    {
        node->left = (BspTreeNode*)malloc(sizeof(BspTreeNode));
        node->left->left = NULL;
        node->left->right= NULL;
        node->left->items = leftItems;
        node->left->numItems= numLeftItems;
        //if (depth < 2)
            subdivideNode(node->left, depth+1);
    }
    
    if (numRightItems)
    {
        node->right = (BspTreeNode*)malloc(sizeof(BspTreeNode));
        node->right->left = NULL;
        node->right->right= NULL;
        node->right->items = rightItems;
        node->right->numItems= numRightItems;
        //if (depth < 2)
            subdivideNode(node->right, depth+1);
    }

}

void freeBspTree(BspTreeNode *root)
{
    if (root->left)
        freeBspTree(root->left);
        
    if (root->right)
        freeBspTree(root->right);

    free(root->items);        
    free(root);
}

BspTreeNode* buildBspTree( Rectangle* items, int numItems)
{
    BspTreeNode *root = (BspTreeNode*)malloc(sizeof(BspTreeNode));
    root->numItems = numItems;
    root->left = NULL;
    root->right= NULL;
    root->items =  (Rectangle*)malloc(sizeof(Rectangle) * numItems);
    memcpy( root->items, items, sizeof(Rectangle) * numItems);
    
    printf("root size: %d\n", root->numItems);
    
    subdivideNode(root, 0);

    int l = root->left ? root->left->numItems : 0;
    int r = root->right ? root->right->numItems : 0;
    printf("node sizes (max/L/C/R): %d/%d/%d/%d\n", 
        ( l > r ? l : r)+root->numItems,l, root->numItems, r);
    return root;
}

void performPhotonMappingNative(Geometry *geo, int numSamplesPerArea)
{
    BspTreeNode *root = buildBspTree(geo->walls, geo->numWalls);

    for ( int i = 0; i < geo->numWindows; i++)
    {
        Vector3 xDir= getWidthVector(  &geo->windows[i]);
        Vector3 yDir= getHeightVector( &geo->windows[i]);
        
        float area = length(xDir) * length(yDir);
        uint64_t numSamples = (numSamplesPerArea * area);
        
        photonmap(&geo->windows[i], root, geo->texels, 1/*true*/, numSamples);
    }

    for ( int i = 0; i < geo->numLights; i++)
    {
        Vector3 xDir= getWidthVector(  &geo->lights[i]);
        Vector3 yDir= getHeightVector( &geo->lights[i]);
        
        float area = length(xDir) * length(yDir);
        uint64_t numSamples = (numSamplesPerArea * area);
        photonmap(&geo->lights[i], root, geo->texels, 0/*false*/, numSamples);
    }
    freeBspTree(root);
//    free(root);
}

void performAmbientOcclusionNativeOnWall(Geometry* geo, const BspTreeNode *root, Rectangle* wall)
{
    Vector3 b1, b2;
    createBase( wall->n, &b1, &b2);

    for (int j = 0; j < getNumTiles(wall); j++)
    {        
        geo->texels[wall->lightmapSetup.s[0] + j] = vec3(0,0,0);
        //printf("normal: %f, %f, %f\n", wall->n.s[0], wall->n.s[1], wall->n.s[2]);

        float distSum = 0;
        
        float lightSum= 0;
        float facSum  = 0;
        for (int k = 0; k < geoSphere4NumVectors; k++)
        {
            float fac = geoSphere4[k].s[2]; //== dot product between the vector and (0,0,1), == cosine between surface normal and light direction
            Vector3 dir = transformToOrthoNormalBase( geoSphere4[k], b1, b2, wall->n);
            assert(fabsf(length(dir) - 1.0f) < 1E-6);
            
            Vector3 pos = getTileCenter(wall, j);
            pos = add(pos, mul(dir, 1E-5));

            float dist = INFINITY;
            Rectangle* target = NULL;
            int hasHit = findClosestIntersection(pos, dir, root, &dist, 0, &target, 0);
            if (!hasHit)    //hit a window/light source
            {
                lightSum += fac;
                dist = 10;
            }

            distSum += /*dist / ( dist + 1 )*/dist * fac;
            facSum += fac;
               
        }

        distSum /= (facSum*1.5);
        geo->texels[wall->lightmapSetup.s[0] + j] = 
            vec3( distSum, distSum, distSum);
    }
}


void performAmbientOcclusionNative(Geometry *geo)
{
    BspTreeNode* root = buildBspTree(geo->walls, geo->numWalls);

    for (int i = 0; i < geo->numWalls; i++)
    {
        printf("processing wall %d/%d\n", i+1, geo->numWalls);
        performAmbientOcclusionNativeOnWall(geo, root, &geo->walls[i]);
    }
    freeBspTree(root);
//    free(root);
}

void pertubeGeoSphere(Vector3* geoSphere, int numVectors) {
    float arc = rand() / (float)RAND_MAX;
    for (int i = 0; i < numVectors; i++)
    {
        Vector3 v = geoSphere[i];
        
        float x = cos(arc) * v.s[0] - sin(arc) * v.s[1];
        v.s[1]  = sin(arc) * v.s[0] + cos(arc) * v.s[1];
        
        v.s[0] = x;
        geoSphere[i] = normalized(v);
        
    }
}


void performRadiosityNative(Geometry *geo)
{
    /*
    for (int i = 0; i < 100; i++)
    {
        Vector3 v = getCosineDistributedRandomRay( vec3(0,0,1));
        printf("%f, %f, %f\n", v.s[0], v.s[1], v.s[2]);
    }
    exit(0);*/
    int numRects = geo->numWalls + geo->numWindows + geo->numLights;
    Rectangle* rects = malloc( numRects * sizeof(Rectangle));
    memcpy(rects,                 geo->walls,   geo->numWalls *   sizeof(Rectangle));
    memcpy(&rects[geo->numWalls], geo->windows, geo->numWindows * sizeof(Rectangle));
    memcpy(&rects[geo->numWalls+geo->numWindows], geo->lights, geo->numLights * sizeof(Rectangle));
    int numTexels = geo->numTexels;
    for (int i = geo->numWalls; i < geo->numWalls + geo->numWindows; i++)
    {
        rects[i].lightmapSetup.s[0] = numTexels;
        numTexels += getNumTiles(&rects[i]);
    }
    int firstWindowTexel = geo->numTexels;
    int firstLightTexel  = numTexels;

    for (int i = geo->numWalls+ geo->numWindows; i < geo->numWalls + geo->numWindows + geo->numLights; i++)
    {
        rects[i].lightmapSetup.s[0] = numTexels;
        numTexels += getNumTiles(&rects[i]);
    }
    
    
    printf("%d/%d texels\n", geo->numTexels, numTexels);
    
    Vector3 *srcTexels = malloc( numTexels * sizeof(Vector3));
    Vector3 *destTexels = malloc( numTexels * sizeof(Vector3));
    
    
    for (int i = 0; i < numTexels; i++)
    {
        if (i < firstWindowTexel) //is a wall texel
            srcTexels[i] = vec3(0,0,0);
        else if (i < firstLightTexel) //is a window texel
            srcTexels[i] = vec3(30,30,30);
        else
            srcTexels[i] = vec3(28,28,32);
        
        destTexels[i] = vec3(0,0,0);
    }
        
    
    BspTreeNode* root = buildBspTree(rects, numRects);


    int geoSphereNumVectors = 100;
    
    printf("\n\e[1A\e7");
    //while(1);
    
    for (int depth = 0; depth < 8; depth++)
    {
        for (int i = 0; i < geo->numWalls; i++)
        {
            if ( (i+1)% 10 == 0)
                printf("\e8processing wall #%d %d/%d\e[K\n", depth, i+1, geo->numWalls);
            Rectangle* wall = &geo->walls[i];

            Vector3 b1, b2;
            createBase( wall->n, &b1, &b2);


            for (int j = 0; j < getNumTiles(wall); j++)
            {        

                int texelIdx = wall->lightmapSetup.s[0] + j;
                geo->texels[texelIdx] = vec3(0,0,0);
                //printf("normal: %f, %f, %f\n", wall->n.s[0], wall->n.s[1], wall->n.s[2]);

                for (int k = 0; k < geoSphereNumVectors; k++)
                {
                    Vector3 dir = getCosineDistributedRandomRay(wall->n);
                    //float fac = dot(dir, wall->n);

                    assert(fabsf(length(dir) - 1.0f) < 1E-6);
                    
                    Vector3 pos = getTileCenter(wall, j);
                    pos = add(pos, mul(dir, 1E-5));

                    float dist = INFINITY;
                    Rectangle* target = NULL;
                    int hasHit = findClosestIntersection(pos, dir, root, &dist, 0, &target, 0);
                    assert(hasHit);
                    if (!hasHit)
                        continue;
                        
                    //float fac = -dot(target->n, dir);
                    //assert(fac >= 0);

                    assert(target);
                        
                    Vector3 hitPos = add (pos, mul(dir, dist));
                    int srcTexelId = target->lightmapSetup.s[0] + getTileIdAt( target, hitPos);

                    //printf("target is %d\n", srcTexelId);
                    //printf("%d\n", srcTexelId);
                    //if (target->lightmapSetup.s[0] > geo->numTexels)
                    //    printf("#\n");

                    inc( &destTexels[texelIdx], 
                         mul( srcTexels[srcTexelId], 0.5 * 1.0/geoSphereNumVectors) );
                }
            }
        }
        for (int i = 0; i < numTexels; i++)
        {
            srcTexels[i] = add( mul(srcTexels[i], 0.5), destTexels[i]);
            destTexels[i] = vec3(0,0,0);
        }

    }
    
    // copy back only those texels corresponding to the walls (omitted window texels)
    for (int i = 0; i < geo->numTexels; i++)
        geo->texels[i] = srcTexels[i];
        
    free(rects);
    free(srcTexels);
    free(destTexels);
    
#if 0
    Vector3* texels2 = malloc(geo->numTexels * sizeof(Vector3));
    for (int i = 0; i < geo->numTexels; i++)
        texels2[i] = vec3(0,0,0);

    
    for (int depth = 0; depth < 1; depth++)
    {
        for (int i = 0; i < geo->numWalls; i++)
        {
            if (! (i % 10))
                printf("processing wall %d/%d\n", i+1, geo->numWalls);
            Rectangle* wall = &geo->walls[i];

            Vector3 b1, b2;
            createBase( wall->n, &b1, &b2);

            for (int j = 0; j < getNumTiles(wall); j++)
            {        
                /*int geoSphereNumVectors = geoSphere3NumVectors;
                Vector3 *geoSphere = malloc( geoSphereNumVectors * sizeof(Vector3));
                memcpy( geoSphere, geoSphere3, geoSphereNumVectors * sizeof(Vector3));

                float scaleFactor = 0;
                for (int i = 0; i < geoSphereNumVectors; i++)
                    scaleFactor += geoSphere[i].s[2]; //== dot product between the vector and (0,0,1), == cosine between surface normal and light direction
                
                scaleFactor = 1/scaleFactor;

                pertubeGeoSphere( geoSphere, geoSphereNumVectors);*/



                int texelIdx = wall->lightmapSetup.s[0] + j;
                //printf("normal: %f, %f, %f\n", wall->n.s[0], wall->n.s[1], wall->n.s[2]);

                for (int k = 0; k < geoSphereNumVectors; k++)
                {
                    Vector3 dir = getCosineDistributedRandomRay(wall->n);
                    //float fac;// = dot(dir, wall->n);

                    assert(fabsf(length(dir) - 1.0f) < 1E-6);
                    
                    Vector3 pos = getTileCenter(wall, j);
                    pos = add(pos, mul(dir, 1E-5));

                    float dist = INFINITY;
                    Rectangle* target = NULL;
                    int hasHit = findClosestIntersection(pos, dir, root, &dist, 0, &target, 0);
                    
                    //int hasWindowHit = findClosestIntersection(pos, dir, rootWindows, &dist, 0, &target, 0);
                    
                    if (!hasHit)
                        continue;
                        
                    float fac = -dot(target->n, dir);
                    //if (fac < 0)
                    //    continue;
                    assert(fac >= 0);

                    assert(target);
                    Vector3 hitPos = add (pos, mul(dir, dist));
                    
                    int srcTexelId = getTileIdAt( target, hitPos);
                    
                    texels2[texelIdx] = add(texels2[texelIdx], vec3(1.0/geoSphereNumVectors, 1.0/geoSphereNumVectors, 1.0/geoSphereNumVectors));
                    //add(texels2[texelIdx],
                    //        mul( geo->texels[srcTexelId], fac/geoSphereNumVectors));
                }
                
                //free (geoSphere);


            }
        }

        for (int i = 0; i < geo->numTexels; i++)
        {
            geo->texels[i] = texels2[i];/*add( geo->texels[i], texels2[i]);
            texels2[i] = vec3(0,0,0);*/
        }
     
    }        
    free (texels2);

    freeBspTree(rootWindows);

    free(rootWindows);
#endif

    freeBspTree(root);
//    free(root);

}

