
#include <math.h>

#include "vector3_cl.h"
#include "rectangle.h"

#include <stdio.h> //for printf()
#include <string.h> //for memcpy()
#include <stdlib.h>

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

/*
float intersects( const Rectangle *rect, const Vector3 ray_src, const Vector3 ray_dir, const float closestDist) 
{
    //if (dot(ray_dir,n) > 0) return -1; //backface culling
    float denom = dot(rect->n, ray_dir);
    if (denom >= 0) // == 0 > ray lies on plane; >0 --> is a backface
        return -1;
        
    //float fac = n.dot( pos - ray_src ) / denom;
    float fac = dot(rect->n, sub(rect->pos, ray_src)) / denom;
    if (fac < 0) 
        return -1;    //is behind camera, cannot be hit
    
    Vector3 ray = mul(ray_dir, fac);
    
    //early termination: if further away than the closest hit (so far), we can ignore this hit
    //N.B.: dot(a,a) = squaredLength(a);
    if (closestDist * closestDist < dot(ray, ray) )
        return -1;
    
    Vector3 pDir = sub( add(ray_src, ray), rect->pos);
    
    float widthLength = length(rect->width);
    float dx = dot( div_vec3(rect->width, widthLength),  pDir);
    if (dx < 0 || dx > widthLength)
        return -1;
        
    float heightLength= length(rect->height);
    float dy = dot( div_vec3(rect->height, heightLength), pDir);
    if  ( dy < 0 || dy > heightLength )
        return -1;
        
    return fac;

    //return select(-1.0f, fac,dx == clamp(dx, 0.0f, widthLength) && dy == clamp(dy, 0.0f, heightLength));

}*/

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
    
    
    float pos = getDistance( &(node->plane), ray_pos);
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
            float planeDist = distanceToPlane( node->plane.n, node->plane.pos, ray_pos, ray_dir);
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
            float planeDist = distanceToPlane( node->plane.n, node->plane.pos, ray_pos, ray_dir);
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
        if (pos.s[2] > 0.0005 || rand()/(double)RAND_MAX > 0.75)
        {   //diffuse reflection
            ray_dir = getCosineDistributedRandomRay(hitObj->n);

            //hack: make floor slightly brownish
            if (pos.s[2] < 1E-5f)
            {
                //printf("%s\n", "Beep");
                //lightColor.s0 *= 0.0f;
                lightColor.s[0] *= 1.0f;
                lightColor.s[1] *= 0.85f;
                lightColor.s[2] *= 0.7f;
            }
            lightColor = mul( lightColor, 0.9f);
        } else
        {   //perfect reflection
        
            ray_dir = sub(ray_dir, mul(hitObj->n, 2* (dot(hitObj->n,ray_dir))));
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
    
    for (int i = 0; i < numSamples; i++)
    {
        if (i % 100000 == 0)
            printf("%d samples, %f %% done.\n", i, (i/(double)numSamples*100));
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
    {
        freeBspTree(root->left);
        free(root->left);
    }
        
    if (root->right)
    {
        freeBspTree(root->right);
        free(root->right);
    }

    free(root->items);        
}

BspTreeNode buildBspTree( Rectangle* items, int numItems)
{
    BspTreeNode root = { .numItems = numItems, .left = NULL, .right=NULL };
    root.items =  (Rectangle*)malloc(sizeof(Rectangle) * numItems);
    memcpy( root.items, items, sizeof(Rectangle) * numItems);
    
    printf("root size: %d\n", root.numItems);
    
    subdivideNode(&root, 0);

    int l = root.left ? root.left->numItems : 0;
    int r = root.right ? root.right->numItems : 0;
    printf("node sizes (max/L/C/R): %d/%d/%d/%d\n", 
        ( l > r ? l : r)+root.numItems,l, root.numItems, r);
    return root;
}

void performPhotonMappingNative(Geometry *geo, Vector3* lightColors, int numSamplesPerArea __attribute__ ((unused)))
{
    BspTreeNode root = buildBspTree(geo->walls, geo->numWalls);

    for ( int i = 0; i < geo->numWindows; i++)
    {
        Vector3 xDir= getWidthVector(  &geo->windows[i]);
        Vector3 yDir= getHeightVector( &geo->windows[i]);
        
        float area = length(xDir) * length(yDir);
        uint64_t numSamples = (numSamplesPerArea * area);
        
        photonmap(&geo->windows[i], &root, lightColors, 1/*true*/, numSamples);
    }

    for ( int i = 0; i < geo->numLights; i++)
    {
        Vector3 xDir= getWidthVector(  &geo->lights[i]);
        Vector3 yDir= getHeightVector( &geo->lights[i]);
        
        float area = length(xDir) * length(yDir);
        uint64_t numSamples = (numSamplesPerArea * area);
        photonmap(&geo->lights[i], &root, lightColors, 0/*false*/, numSamples);
    }
    freeBspTree(&root);
}


void performAmbientOcclusionNative(Geometry *geo, Vector3* lightColors)
{
    BspTreeNode root = buildBspTree(geo->walls, geo->numWalls);

    /*almost equally-distributed points on a geodesic sphere, generated by a python script.
      those points with z-value 0.0 (=vectors perpendicular to the surface normal) have 
      been omitted.
      
     */
/*
    static const Vector3 directionalVectors[] = {
        { .s= { -0.33141357403559163 , -0.19134171618254506 , 0.9238795325112867 }},
        { .s= { 0.8001031451912656 , -0.4619397662556432 , 0.3826834323650898 }},
        { .s= { 0.33141357403559174 , -0.19134171618254478 , 0.9238795325112867 }},
        { .s= { -2.2628522245754005e-16 , 0.9238795325112867 , 0.3826834323650897 }},
        { .s= { -0.8985425047144445 , 0.08900750509045682 , 0.429766251884748 }},
        { .s= { -9.373040810652596e-17 , 0.3826834323650897 , 0.9238795325112867 }},
        { .s= { 0 , 0 , 1 }},
        { .s= { -0.8001031451912652 , -0.4619397662556438 , 0.3826834323650897 }},
        { .s= { -0.6123724356957942 , -0.35355339059327406 , 0.7071067811865475 }},
        { .s= { -1.7319121124709866e-16 , 0.7071067811865475 , 0.7071067811865475 }},
        { .s= { -0.3721884918214132 , -0.8226643880080364 , 0.42976625188474793 }},
        { .s= { 0.37218849182141384 , -0.8226643880080362 , 0.429766251884748 }},
        { .s= { 0.3872983346207416 , 0.2236067977499791 , 0.894427190999916 }},
        { .s= { -0.5263540128930312 , 0.7336568829175789 , 0.42976625188474793 }},
        { .s= { -0.38729833462074176 , 0.2236067977499788 , 0.894427190999916 }},
        { .s= { 1.4043333874306804e-16 , -0.44721359549995804 , 0.8944271909999157 }},
        { .s= { 0.6123724356957945 , -0.35355339059327356 , 0.7071067811865475 }},
        { .s= { 0.8985425047144444 , 0.08900750509045749 , 0.42976625188474793 }},
        { .s= { 0.5263540128930307 , 0.7336568829175794 , 0.42976625188474793 }},
    };

    static const int numDirectionalVectors = (int)(sizeof(directionalVectors)/sizeof(Vector3)) ;
*/

    static const Vector3 directionalVectors[] = { 
        { .s= { 0.26351558835392164 , 0.9433357047711144 , 0.20168609966596981 }},
        { .s= { 5.883624753340643e-17 , -0.20280301033360357 , 0.9792195560749531 }},
        { .s= { 0.3872983346207416 , 0.2236067977499791 , 0.894427190999916 }},
        { .s= { 0.19805987534297256 , 0.31004979021105705 , 0.9298609645367449 }},
        { .s= { 0.8001031451912656 , -0.4619397662556432 , 0.3826834323650898 }},
        { .s= { -0.6851948904517209 , -0.6998790461932567 , 0.2016860996659698 }},
        { .s= { -0.16948105708932468 , -0.32654977862292217 , 0.9298609645367449 }},
        { .s= { -2.2628522245754005e-16 , 0.9238795325112867 , 0.3826834323650897 }},
        { .s= { 0.19493713931417053 , -0.9546372054367136 , 0.22509401971618356 }},
        { .s= { -0.26351558835392214 , 0.9433357047711143 , 0.2016860996659698 }},
        { .s= { -0.3721884918214132 , -0.8226643880080364 , 0.42976625188474793 }},
        { .s= { 0.20466098062791785 , 0.49181919136160296 , 0.8463024081360266 }},
        { .s= { -0.7292715016488932 , 0.6461391175054942 , 0.22509401971618354 }},
        { .s= { -1.3607546151380102e-16 , 0.5555702330196022 , 0.8314696123025452 }},
        { .s= { -0.4811379353814155 , -0.27778511650980137 , 0.8314696123025452 }},
        { .s= { 0.37218849182141384 , -0.8226643880080362 , 0.429766251884748 }},
        { .s= { -0.7675944163066124 , 0.44317084288307595 , 0.46303176572802773 }},
        { .s= { 1.4043333874306804e-16 , -0.44721359549995804 , 0.8944271909999157 }},
        { .s= { -0.16895317489845357 , -0.09754516100806422 , 0.9807852804032304 }},
        { .s= { 0.5267105587269101 , 0.8209346292117787 , 0.22055004395035935 }},
        { .s= { -2.402231108593311e-16 , 0.9807852804032304 , 0.19509032201612825 }},
        { .s= { -0.1949371393141699 , -0.9546372054367138 , 0.22509401971618356 }},
        { .s= { 0.9242086409630631 , 0.3084980879312195 , 0.22509401971618354 }},
        { .s= { -0.36754093243229746 , 0.016499988411864796 , 0.9298609645367449 }},
        { .s= { 0.1694810570893249 , -0.326549778622922 , 0.9298609645367449 }},
        { .s= { 0.8493849684870416 , -0.49039264020161505 , 0.19509032201612828 }},
        { .s= { -0.32359742347390913 , -0.42315120406801354 , 0.8463024081360266 }},
        { .s= { -0.5282584041018272 , -0.06866798729358986 , 0.8463024081360268 }},
        { .s= { 0.5156728241420288 , -0.6160548593238783 , 0.5954476876643496 }},
        { .s= { -1.7319121124709866e-16 , 0.7071067811865475 , 0.7071067811865475 }},
        { .s= { -0.5263540128930312 , 0.7336568829175789 , 0.42976625188474793 }},
        { .s= { -0.8985425047144445 , 0.08900750509045682 , 0.429766251884748 }},
        { .s= { 0.32359742347390935 , -0.4231512040680134 , 0.8463024081360268 }},
        { .s= { -0.6123724356957942 , -0.35355339059327406 , 0.7071067811865475 }},
        { .s= { -0.5267105587269106 , 0.8209346292117784 , 0.22055004395035938 }},
        { .s= { 0.9743055231072141 , 0.04567740969311085 , 0.22055004395035938 }},
        { .s= { -0.17563255891285864 , 0.10140150516680164 , 0.9792195560749531 }},
        { .s= { 0.44759496438030394 , -0.866612038904889 , 0.22055004395035938 }},
        { .s= { 0.33141357403559174 , -0.19134171618254478 , 0.9238795325112867 }},
        { .s= { -0.1988124368765839 , -0.6783324632140645 , 0.7073462266055671 }},
        { .s= { -0.8001031451912652 , -0.4619397662556438 , 0.3826834323650897 }},
        { .s= { -0.9242086409630633 , 0.3084980879312188 , 0.22509401971618356 }},
        { .s= { -0.488046926916761 , 0.511342852530444 , 0.7073462266055671 }},
        { .s= { 0.1756325589128586 , 0.1014015051668018 , 0.9792195560749531 }},
        { .s= { 0.2742115292946347 , 0.8635169186573036 , 0.4232570949133557 }},
        { .s= { 0.6123724356957945 , -0.35355339059327356 , 0.7071067811865475 }},
        { .s= { -0.7200738067288022 , -0.41573480615127306 , 0.5555702330196022 }},
        { .s= { -9.373040810652596e-17 , 0.3826834323650897 , 0.9238795325112867 }},
        { .s= { 0.767594416306612 , 0.44317084288307657 , 0.46303176572802773 }},
        { .s= { 0.19881243687658437 , -0.6783324632140643 , 0.707346226605567 }},
        { .s= { -0.20466098062791815 , 0.49181919136160285 , 0.8463024081360268 }},
        { .s= { 0.48804692691676066 , 0.5113428525304444 , 0.7073462266055671 }},
        { .s= { -0.19805987534297276 , 0.31004979021105694 , 0.9298609645367449 }},
        { .s= { 0.7292715016488928 , 0.6461391175054948 , 0.2250940197161835 }},
        { .s= { 0.27568274622831285 , 0.7546131954102014 , 0.5954476876643495 }},
        { .s= { 0.7913555703703417 , -0.1385583360863227 , 0.5954476876643496 }},
        { .s= { -0.2756827462283133 , 0.7546131954102012 , 0.5954476876643495 }},
        { .s= { -0.5156728241420283 , -0.6160548593238787 , 0.5954476876643496 }},
        { .s= { -0.8493849684870413 , -0.49039264020161566 , 0.19509032201612825 }},
        { .s= { 0.16895317489845363 , -0.09754516100806408 , 0.9807852804032304 }},
        { .s= { 0.9487104788056433 , -0.24345665857785775 , 0.20168609966596981 }},
        { .s= { -0.33141357403559163 , -0.19134171618254506 , 0.9238795325112867 }},
        { .s= { 0.7200738067288024 , -0.41573480615127245 , 0.5555702330196023 }},
        { .s= { 0.8985425047144444 , 0.08900750509045749 , 0.42976625188474793 }},
        { .s= { -0.8849333528022029 , -0.1942843089489175 , 0.4232570949133558 }},
        { .s= { 0.6868593637933449 , 0.16698961068362034 , 0.7073462266055671 }},
        { .s= { 0 , 0 , 1 }},
        { .s= { 0.4811379353814157 , -0.277785116509801 , 0.8314696123025453 }},
        { .s= { -0.6107218235075678 , -0.6692326097083867 , 0.4232570949133557 }},
        { .s= { -0.9487104788056432 , -0.24345665857785853 , 0.20168609966596981 }},
        { .s= { -0.44759496438030344 , -0.8666120389048892 , 0.22055004395035938 }},
        { .s= { 0.36754093243229746 , 0.016499988411865094 , 0.9298609645367449 }},
        { .s= { 3.2894356973622565e-16 , -0.8863416857661524 , 0.4630317657280277 }},
        { .s= { 0.8849333528022031 , -0.19428430894891685 , 0.4232570949133558 }},
        { .s= { -0.7913555703703415 , -0.13855833608632329 , 0.5954476876643496 }},
        { .s= { -0.9743055231072142 , 0.04567740969311014 , 0.22055004395035943 }},
        { .s= { -0.2742115292946352 , 0.8635169186573034 , 0.4232570949133558 }},
        { .s= { -4.778334768033558e-17 , 0.19509032201612825 , 0.9807852804032304 }},
        { .s= { 0.6107218235075682 , -0.6692326097083863 , 0.42325709491335584 }},
        { .s= { 0.5282584041018272 , -0.06866798729358943 , 0.8463024081360268 }},
        { .s= { -0.38729833462074176 , 0.2236067977499788 , 0.894427190999916 }},
        { .s= { -0.686859363793345 , 0.1669896106836198 , 0.7073462266055672 }},
        { .s= { 0.6851948904517214 , -0.6998790461932561 , 0.20168609966596981 }},
        { .s= { -2.0365131985892053e-16 , 0.8314696123025452 , 0.5555702330196022 }},
        { .s= { 0.5263540128930307 , 0.7336568829175794 , 0.42976625188474793 }},
    };
    static const int numDirectionalVectors = (int)(sizeof(directionalVectors)/sizeof(Vector3)) ;

    for (int i = 0; i < geo->numWalls; i++)
    {

        Rectangle *wall = &geo->walls[i];
        //printf("Position: %f, %f, %f\n", wall->pos.s[0],  wall->pos.s[1],   wall->pos.s[2]);
        //printf("Width: %f, %f, %f\n", wall->width.s[0],   wall->width.s[1], wall->width.s[2]);
        //printf("Height: %f, %f, %f\n", wall->height.s[0], wall->height.s[1],wall->height.s[2]);
        //printf("numTiles: %d/%d\n", wall->lightmapSetup.s[1], wall->lightmapSetup.s[2]);

        Vector3 b1, b2;
        createBase( wall->n, &b1, &b2);

        for (int j = 0; j < getNumTiles(wall); j++)
        {
            lightColors[wall->lightmapSetup.s[0] + j] = vec3(0,0,0);
            //printf("normal: %f, %f, %f\n", wall->n.s[0], wall->n.s[1], wall->n.s[2]);

            float distSum = 0;
            for (int k = 0; k < numDirectionalVectors; k++)
            {
                float fac = directionalVectors[k].s[2]; //== dot product between the vector and (0,0,0), == cosine between surface normal and light direction
                Vector3 dir = transformToOrthoNormalBase( directionalVectors[k], b1, b2, wall->n);
                //Vector3 dir = getCosineDistributedRandomRay( wall->n);
                //float fac = dot( wall->n, 
                assert(fabsf(length(dir) - 1.0f) < 1E-6);
                //printf("direction: %f, %f, %f", dir.s[0], dir.s[1], dir.s[2]);
                
                Vector3 pos = getTileCenter(wall, j);
                float dist = INFINITY;
                Rectangle* target = NULL;
                int hasHit = findClosestIntersection(pos, dir, &root, &dist, 0, &target, 0);
                if (!hasHit)    //hit a window/light source
                {
                    
                    dist = 10;
                //    distSum += 1;//fac;
                }

                //printf(", hit at %f\n", dist);
                
                if (dist > 1)
                    distSum += fac;
                    
                //distSum  += fac * log(dist+1);//fac * //sqrt(dist);
                //Vector3 col = hasHit ?
                //Vector3 col = vec3(dist, dist, dist);// : vec3(0, 0, 0);

                //col = mul(col, 1.0/*numSamplesPerArea*/);
                //lightColors[wall->lightmapSetup.s[0] + j] = add(lightColors[wall->lightmapSetup.s[0] + j], mul(col, fac));
                    
            }

            lightColors[wall->lightmapSetup.s[0] + j] = 
                mul( vec3( distSum, distSum, distSum), 0.03);
        }
    }
    freeBspTree(&root);

}
