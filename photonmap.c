
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
*/
/*
    static const Vector3 directionalVectors[] = { 
        { .s= { 8.659560562354933e-17 , -0.7071067811865475 , 0.7071067811865475 }},
        { .s= { -0.2088465988715235 , 0.40461504459635395 , 0.890320034496634 }},
        { .s= { 0.8903200344966341 , -0.4046150445963539 , 0.2088465988715234 }},
        { .s= { -1.7319121124709866e-16 , 0.7071067811865475 , 0.7071067811865475 }},
        { .s= { 0.2113248654051872 , -0.5773502691896257 , 0.7886751345948129 }},
        { .s= { -0.1987568534155134 , 0.19875685341551333 , 0.9596829822606673 }},
        { .s= { -0.6396021490668312 , -0.426401432711221 , 0.6396021490668312 }},
        { .s= { 0.4082482904638632 , -0.816496580927726 , 0.408248290463863 }},
        { .s= { 4.686520405326298e-17 , -0.3826834323650897 , 0.9238795325112867 }},
        { .s= { 0.4046150445963541 , -0.890320034496634 , 0.20884659887152338 }},
        { .s= { -0.577350269189626 , 0.7886751345948128 , 0.2113248654051871 }},
        { .s= { 0.4082482904638629 , 0.8164965809277261 , 0.4082482904638631 }},
        { .s= { 0.5773502691896256 , 0.788675134594813 , 0.21132486540518713 }},
        { .s= { -1.3607546151380102e-16 , 0.5555702330196022 , 0.8314696123025452 }},
        { .s= { 0.426401432711221 , -0.6396021490668312 , 0.6396021490668312 }},
        { .s= { 0.7071067811865475 , 4.3297802811774664e-17 , 0.7071067811865475 }},
        { .s= { -0.788675134594813 , 0.5773502691896255 , 0.2113248654051871 }},
        { .s= { -0.40824829046386296 , -0.4082482904638631 , 0.816496580927726 }},
        { .s= { 6.803773075690051e-17 , -0.5555702330196022 , 0.8314696123025452 }},
        { .s= { -0.9596829822606673 , -0.19875685341551355 , 0.19875685341551336 }},
        { .s= { 0.8164965809277261 , -0.40824829046386296 , 0.408248290463863 }},
        { .s= { 0.1987568534155135 , -0.9596829822606673 , 0.19875685341551336 }},
        { .s= { 1.0182565992946027e-16 , -0.8314696123025452 , 0.5555702330196022 }},
        { .s= { -0.19875685341551333 , -0.19875685341551338 , 0.9596829822606673 }},
        { .s= { 0.5773502691896257 , -0.21132486540518708 , 0.7886751345948129 }},
        { .s= { -0.3826834323650897 , -7.029780607989446e-17 , 0.9238795325112867 }},
        { .s= { -0.20884659887152332 , -0.40461504459635395 , 0.890320034496634 }},
        { .s= { 0.6396021490668314 , -0.6396021490668312 , 0.42640143271122083 }},
        { .s= { -2.402231108593311e-16 , 0.9807852804032304 , 0.19509032201612825 }},
        { .s= { -0.7071067811865475 , -1.2989340843532398e-16 , 0.7071067811865475 }},
        { .s= { -0.404615044596354 , 0.20884659887152332 , 0.890320034496634 }},
        { .s= { 0.4264014327112208 , 0.6396021490668313 , 0.6396021490668313 }},
        { .s= { -0.20884659887152324 , -0.890320034496634 , 0.40461504459635395 }},
        { .s= { -0.2088465988715236 , 0.890320034496634 , 0.40461504459635395 }},
        { .s= { 0.3826834323650897 , 2.343260202663149e-17 , 0.9238795325112867 }},
        { .s= { 0.5773502691896256 , 0.21132486540518716 , 0.788675134594813 }},
        { .s= { -0.42640143271122105 , 0.6396021490668312 , 0.6396021490668312 }},
        { .s= { -0.40824829046386313 , 0.40824829046386296 , 0.816496580927726 }},
        { .s= { 0.890320034496634 , 0.40461504459635406 , 0.20884659887152338 }},
        { .s= { 2.389167384016779e-17 , -0.19509032201612825 , 0.9807852804032304 }},
        { .s= { 0.4082482904638629 , 0.4082482904638631 , 0.8164965809277261 }},
        { .s= { -0.890320034496634 , -0.4046150445963542 , 0.20884659887152338 }},
        { .s= { 0.21132486540518725 , -0.7886751345948129 , 0.5773502691896257 }},
        { .s= { -0.9807852804032304 , -1.8016733314449831e-16 , 0.19509032201612825 }},
        { .s= { 0.6396021490668313 , -0.4264014327112209 , 0.6396021490668312 }},
        { .s= { -0.890320034496634 , -0.20884659887152354 , 0.40461504459635395 }},
        { .s= { 0.19875685341551325 , 0.9596829822606674 , 0.19875685341551338 }},
        { .s= { -0.7886751345948128 , -0.5773502691896258 , 0.2113248654051871 }},
        { .s= { -0.6396021490668314 , 0.4264014327112208 , 0.6396021490668312 }},
        { .s= { -0.4046150445963542 , 0.890320034496634 , 0.20884659887152338 }},
        { .s= { -0.9238795325112867 , -1.6971391684315505e-16 , 0.3826834323650897 }},
        { .s= { -0.5773502691896258 , 0.21132486540518705 , 0.7886751345948129 }},
        { .s= { 0.19875685341551333 , 0.19875685341551338 , 0.9596829822606674 }},
        { .s= { -0.21132486540518705 , -0.5773502691896258 , 0.7886751345948129 }},
        { .s= { 0.7886751345948129 , 0.5773502691896257 , 0.2113248654051871 }},
        { .s= { 0.7886751345948129 , 0.21132486540518716 , 0.5773502691896257 }},
        { .s= { -0.21132486540518697 , -0.788675134594813 , 0.5773502691896257 }},
        { .s= { -0.21132486540518727 , 0.5773502691896257 , 0.7886751345948129 }},
        { .s= { -0.42640143271122083 , -0.6396021490668314 , 0.6396021490668313 }},
        { .s= { 0.19509032201612825 , 1.1945836920083895e-17 , 0.9807852804032304 }},
        { .s= { -0.2113248654051873 , 0.7886751345948129 , 0.5773502691896257 }},
        { .s= { -2.2628522245754005e-16 , 0.9238795325112867 , 0.3826834323650897 }},
        { .s= { -0.5555702330196022 , -1.0205659613535074e-16 , 0.8314696123025452 }},
        { .s= { 0.21132486540518697 , 0.5773502691896257 , 0.788675134594813 }},
        { .s= { -9.373040810652596e-17 , 0.3826834323650897 , 0.9238795325112867 }},
        { .s= { 0.890320034496634 , 0.20884659887152343 , 0.40461504459635395 }},
        { .s= { 0.890320034496634 , -0.20884659887152332 , 0.40461504459635395 }},
        { .s= { 0.788675134594813 , -0.21132486540518705 , 0.5773502691896257 }},
        { .s= { -0.8903200344966341 , 0.4046150445963538 , 0.2088465988715234 }},
        { .s= { -0.8164965809277261 , 0.40824829046386285 , 0.408248290463863 }},
        { .s= { -0.40824829046386324 , 0.816496580927726 , 0.408248290463863 }},
        { .s= { 0.5555702330196022 , 3.4018865378450254e-17 , 0.8314696123025452 }},
        { .s= { 0.40461504459635383 , 0.8903200344966341 , 0.2088465988715234 }},
        { .s= { 0.9238795325112867 , 5.657130561438501e-17 , 0.3826834323650897 }},
        { .s= { 0.4082482904638631 , -0.408248290463863 , 0.816496580927726 }},
        { .s= { -0.19509032201612825 , -3.583751076025168e-17 , 0.9807852804032304 }},
        { .s= { 1.1314261122877003e-16 , -0.9238795325112867 , 0.3826834323650897 }},
        { .s= { -0.5773502691896257 , -0.21132486540518722 , 0.7886751345948129 }},
        { .s= { -0.5773502691896255 , -0.788675134594813 , 0.2113248654051871 }},
        { .s= { 0.4046150445963539 , 0.2088465988715234 , 0.890320034496634 }},
        { .s= { 0.816496580927726 , 0.4082482904638631 , 0.408248290463863 }},
        { .s= { 0.2088465988715232 , 0.890320034496634 , 0.40461504459635395 }},
        { .s= { -0.19875685341551358 , 0.9596829822606673 , 0.19875685341551336 }},
        { .s= { -0.8314696123025452 , -1.527384898941904e-16 , 0.5555702330196022 }},
        { .s= { 0.5773502691896258 , -0.7886751345948128 , 0.2113248654051871 }},
        { .s= { -0.6396021490668314 , 0.6396021490668312 , 0.42640143271122083 }},
        { .s= { 0.21132486540518697 , 0.788675134594813 , 0.5773502691896257 }},
        { .s= { -0.19875685341551325 , -0.9596829822606674 , 0.19875685341551338 }},
        { .s= { -0.40824829046386285 , -0.8164965809277261 , 0.408248290463863 }},
        { .s= { 0.19875685341551338 , -0.19875685341551336 , 0.9596829822606673 }},
        { .s= { -0.890320034496634 , 0.2088465988715232 , 0.40461504459635395 }},
        { .s= { 0 , 0 , 1 }},
        { .s= { 0.40461504459635395 , -0.20884659887152338 , 0.890320034496634 }},
        { .s= { 0.6396021490668312 , 0.42640143271122094 , 0.6396021490668313 }},
        { .s= { 0.788675134594813 , -0.5773502691896256 , 0.2113248654051871 }},
        { .s= { 1.2011155542966555e-16 , -0.9807852804032304 , 0.19509032201612825 }},
        { .s= { 0.9596829822606674 , -0.19875685341551327 , 0.19875685341551338 }},
        { .s= { 0.6396021490668312 , 0.6396021490668313 , 0.4264014327112209 }},
        { .s= { 0.20884659887152351 , -0.890320034496634 , 0.40461504459635395 }},
        { .s= { -0.788675134594813 , 0.21132486540518697 , 0.5773502691896257 }},
        { .s= { -0.9596829822606674 , 0.1987568534155132 , 0.19875685341551338 }},
        { .s= { 0.8314696123025452 , 5.0912829964730134e-17 , 0.5555702330196022 }},
        { .s= { 0.9807852804032304 , 6.005577771483278e-17 , 0.19509032201612825 }},
        { .s= { 0.20884659887152343 , -0.40461504459635395 , 0.890320034496634 }},
        { .s= { -4.778334768033558e-17 , 0.19509032201612825 , 0.9807852804032304 }},
        { .s= { 0.9596829822606673 , 0.19875685341551347 , 0.19875685341551336 }},
        { .s= { 0.20884659887152326 , 0.40461504459635395 , 0.890320034496634 }},
        { .s= { -0.6396021490668312 , -0.6396021490668314 , 0.42640143271122083 }},
        { .s= { -0.40461504459635383 , -0.8903200344966341 , 0.2088465988715234 }},
        { .s= { -0.40461504459635395 , -0.20884659887152343 , 0.890320034496634 }},
        { .s= { -0.7886751345948129 , -0.21132486540518725 , 0.5773502691896257 }},
        { .s= { -2.0365131985892053e-16 , 0.8314696123025452 , 0.5555702330196022 }},
        { .s= { -0.816496580927726 , -0.4082482904638632 , 0.408248290463863 }},
    };
*/

    static const Vector3 directionalVectors[] = { 
        { .s= { 0.49705162538629705 , -0.3124597141037825 , 0.8095113394900795 }},
        { .s= { -0.21372447380823695 , 0.690768358169145 , 0.690768358169145 }},
        { .s= { -0.843745492253799 , 0.2111487784381132 , 0.49346705833873755 }},
        { .s= { -0.5295921058604979 , 0.4218387356406946 , 0.735924101061586 }},
        { .s= { -0.1987568534155134 , 0.19875685341551333 , 0.9596829822606673 }},
        { .s= { -0.20884659887152332 , -0.40461504459635395 , 0.890320034496634 }},
        { .s= { -0.31030659960478946 , -0.8582739368522557 , 0.40874890037998013 }},
        { .s= { -0.49705162538629705 , 0.3124597141037824 , 0.8095113394900795 }},
        { .s= { 0.40874890037998024 , -0.8582739368522556 , 0.3103065996047896 }},
        { .s= { -0.40824829046386313 , 0.40824829046386296 , 0.816496580927726 }},
        { .s= { 0.32178683126090696 , 0.6140553770836691 , 0.7206866372437452 }},
        { .s= { -0.40874890037997996 , -0.8582739368522557 , 0.3103065996047897 }},
        { .s= { 5.772945048824654e-17 , -0.47139673682599764 , 0.881921264348355 }},
        { .s= { -0.32178683126090724 , 0.720686637243745 , 0.6140553770836692 }},
        { .s= { 0.9305184620712355 , 0.20501725355122333 , 0.30348528381273476 }},
        { .s= { 0.5773502691896256 , 0.788675134594813 , 0.21132486540518713 }},
        { .s= { -0.6140553770836693 , 0.7206866372437449 , 0.321786831260907 }},
        { .s= { 0.10628823928135916 , 0.5698152235643941 , 0.8148701867075075 }},
        { .s= { -0.21132486540518697 , -0.788675134594813 , 0.5773502691896257 }},
        { .s= { 0.1979147287015083 , -0.09987861072809527 , 0.9751174407639492 }},
        { .s= { 0.1987568534155135 , -0.9596829822606673 , 0.19875685341551336 }},
        { .s= { 0.9751174407639492 , 0.19791472870150842 , 0.09987861072809527 }},
        { .s= { -0.816496580927726 , -0.4082482904638632 , 0.408248290463863 }},
        { .s= { 0.29028467725446233 , 1.777481004205999e-17 , 0.9569403357322088 }},
        { .s= { -0.8582739368522558 , 0.40874890037997996 , 0.31030659960478973 }},
        { .s= { 0.7543444794845716 , -0.10657418966918722 , 0.6477702898153842 }},
        { .s= { 0.9902528527693809 , 0.09848676962440882 , 0.09848676962440873 }},
        { .s= { 0.6396021490668313 , -0.4264014327112209 , 0.6396021490668312 }},
        { .s= { -0.10657418966918744 , 0.6477702898153842 , 0.7543444794845715 }},
        { .s= { -0.6477702898153842 , 0.10657418966918718 , 0.7543444794845715 }},
        { .s= { -0.2088465988715236 , 0.890320034496634 , 0.40461504459635395 }},
        { .s= { 0.3826834323650897 , 2.343260202663149e-17 , 0.9238795325112867 }},
        { .s= { 0.8148701867075075 , 0.10628823928135937 , 0.5698152235643941 }},
        { .s= { -0.10534740065548343 , -0.4843412518617614 , 0.8685133717566557 }},
        { .s= { -0.49346705833873766 , 0.2111487784381133 , 0.8437454922537989 }},
        { .s= { 0.8148701867075074 , 0.5698152235643941 , 0.1062882392813593 }},
        { .s= { 0.4970516253862971 , -0.8095113394900795 , 0.3124597141037825 }},
        { .s= { -1.7319121124709866e-16 , 0.7071067811865475 , 0.7071067811865475 }},
        { .s= { 0.843745492253799 , -0.2111487784381133 , 0.49346705833873755 }},
        { .s= { -0.9902528527693809 , 0.09848676962440855 , 0.09848676962440873 }},
        { .s= { -0.2934701936702926 , -0.1003184913251062 , 0.9506899840249583 }},
        { .s= { 0.8582739368522557 , -0.31030659960478957 , 0.40874890037998013 }},
        { .s= { -0.9951847266721968 , -1.8281246851191592e-16 , 0.09801714032956059 }},
        { .s= { -0.3958935934845478 , -0.10501865929345938 , 0.9122715296654259 }},
        { .s= { 0.9569403357322088 , 5.859569595647216e-17 , 0.29028467725446233 }},
        { .s= { 0.8437454922537989 , 0.49346705833873766 , 0.21114877843811336 }},
        { .s= { -0.48434125186176136 , -0.8685133717566558 , 0.10534740065548351 }},
        { .s= { 0.19791472870150817 , 0.9751174407639492 , 0.09987861072809529 }},
        { .s= { -0.09848676962440872 , -0.09848676962440875 , 0.9902528527693809 }},
        { .s= { 0.10657418966918712 , 0.6477702898153842 , 0.7543444794845716 }},
        { .s= { -0.8989750671491784 , 0.3097126318413006 , 0.30971263184130077 }},
        { .s= { 0.9122715296654258 , 0.39589359348454783 , 0.1050186592934593 }},
        { .s= { -0.8437454922537989 , -0.49346705833873783 , 0.21114877843811336 }},
        { .s= { -0.5773502691896258 , 0.21132486540518705 , 0.7886751345948129 }},
        { .s= { -0.20501725355122322 , -0.30348528381273476 , 0.9305184620712355 }},
        { .s= { 0.6477702898153842 , 0.10657418966918733 , 0.7543444794845716 }},
        { .s= { 0.3097126318413006 , 0.8989750671491784 , 0.30971263184130077 }},
        { .s= { 0.569815223564394 , 0.8148701867075075 , 0.10628823928135932 }},
        { .s= { -0.30971263184130093 , 0.8989750671491783 , 0.3097126318413007 }},
        { .s= { -0.49705162538629716 , 0.8095113394900795 , 0.3124597141037825 }},
        { .s= { -0.8685133717566558 , 0.4843412518617613 , 0.10534740065548351 }},
        { .s= { 0.5698152235643941 , -0.10628823928135929 , 0.8148701867075075 }},
        { .s= { 0.720686637243745 , 0.6140553770836692 , 0.32178683126090707 }},
        { .s= { -0.9122715296654259 , -0.10501865929345948 , 0.3958935934845478 }},
        { .s= { 0.2934701936702926 , 0.10031849132510616 , 0.9506899840249583 }},
        { .s= { -0.321786831260907 , -0.6140553770836692 , 0.720686637243745 }},
        { .s= { -0.6907683581691452 , 0.690768358169145 , 0.21372447380823678 }},
        { .s= { 0.21132486540518697 , 0.5773502691896257 , 0.788675134594813 }},
        { .s= { -0.10657418966918716 , -0.7543444794845716 , 0.6477702898153842 }},
        { .s= { -0.2111487784381133 , -0.49346705833873766 , 0.8437454922537989 }},
        { .s= { -0.7543444794845714 , -0.6477702898153844 , 0.10657418966918727 }},
        { .s= { 0.09987861072809509 , 0.9751174407639492 , 0.1979147287015083 }},
        { .s= { -1.5538154097031715e-16 , 0.6343932841636454 , 0.773010453362737 }},
        { .s= { 0.4082482904638631 , -0.408248290463863 , 0.816496580927726 }},
        { .s= { 0.21372447380823661 , 0.6907683581691451 , 0.6907683581691451 }},
        { .s= { -0.6477702898153841 , -0.7543444794845717 , 0.10657418966918729 }},
        { .s= { -0.21372447380823667 , -0.6907683581691451 , 0.690768358169145 }},
        { .s= { 1.1314261122877003e-16 , -0.9238795325112867 , 0.3826834323650897 }},
        { .s= { 0.6469966392206306 , -0.5391638660171921 , 0.539163866017192 }},
        { .s= { -0.404615044596354 , 0.20884659887152332 , 0.890320034496634 }},
        { .s= { -0.10031849132510622 , 0.2934701936702926 , 0.9506899840249583 }},
        { .s= { 0.614055377083669 , 0.7206866372437452 , 0.32178683126090707 }},
        { .s= { -0.40874890037998024 , 0.3103065996047896 , 0.8582739368522557 }},
        { .s= { 0.29347019367029265 , -0.10031849132510613 , 0.9506899840249583 }},
        { .s= { 0.10501865929345912 , 0.9122715296654259 , 0.3958935934845478 }},
        { .s= { 0.30348528381273465 , 0.9305184620712356 , 0.2050172535512233 }},
        { .s= { -0.29028467725446233 , -5.3324430126179966e-17 , 0.9569403357322088 }},
        { .s= { -0.49346705833873755 , -0.21114877843811344 , 0.8437454922537989 }},
        { .s= { -0.2113248654051873 , 0.7886751345948129 , 0.5773502691896257 }},
        { .s= { -0.5698152235643942 , 0.8148701867075073 , 0.1062882392813593 }},
        { .s= { 0.10628823928135914 , 0.8148701867075075 , 0.5698152235643941 }},
        { .s= { 0.7206866372437452 , -0.321786831260907 , 0.6140553770836691 }},
        { .s= { 0.4843412518617614 , -0.10534740065548347 , 0.8685133717566557 }},
        { .s= { 0 , 0 , 1 }},
        { .s= { -0.30348528381273465 , -0.9305184620712356 , 0.2050172535512233 }},
        { .s= { 0.09848676962440886 , -0.9902528527693809 , 0.09848676962440873 }},
        { .s= { -0.3103065996047898 , 0.40874890037998013 , 0.8582739368522557 }},
        { .s= { 0.5698152235643941 , 0.10628823928135935 , 0.8148701867075075 }},
        { .s= { 0.21132486540518725 , -0.7886751345948129 , 0.5773502691896257 }},
        { .s= { -0.8437454922537991 , 0.49346705833873744 , 0.21114877843811336 }},
        { .s= { 0.912271529665426 , -0.3958935934845476 , 0.10501865929345933 }},
        { .s= { -0.8095113394900796 , 0.4970516253862968 , 0.31245971410378254 }},
        { .s= { 0.890320034496634 , 0.40461504459635406 , 0.20884659887152338 }},
        { .s= { -0.8095113394900796 , 0.3124597141037823 , 0.497051625386297 }},
        { .s= { 0.7543444794845716 , -0.6477702898153841 , 0.10657418966918727 }},
        { .s= { 0.7886751345948129 , 0.21132486540518716 , 0.5773502691896257 }},
        { .s= { 0.30348528381273493 , -0.9305184620712355 , 0.20501725355122327 }},
        { .s= { -0.4218387356406948 , 0.5295921058604978 , 0.735924101061586 }},
        { .s= { -0.40874890037998013 , -0.31030659960478973 , 0.8582739368522557 }},
        { .s= { 0.20884659887152326 , 0.40461504459635395 , 0.890320034496634 }},
        { .s= { -0.6396021490668312 , -0.6396021490668314 , 0.42640143271122083 }},
        { .s= { -0.690768358169145 , -0.6907683581691452 , 0.21372447380823678 }},
        { .s= { -0.40461504459635395 , -0.20884659887152343 , 0.890320034496634 }},
        { .s= { 0.10031849132510619 , -0.29347019367029265 , 0.9506899840249583 }},
        { .s= { -0.1979147287015082 , -0.9751174407639492 , 0.09987861072809529 }},
        { .s= { 0.09848676962440875 , -0.09848676962440873 , 0.9902528527693809 }},
        { .s= { 0.9506899840249583 , 0.10031849132510623 , 0.29347019367029265 }},
        { .s= { 0.7543444794845715 , 0.6477702898153842 , 0.10657418966918727 }},
        { .s= { 0.10031849132510608 , 0.29347019367029265 , 0.9506899840249583 }},
        { .s= { 8.659560562354933e-17 , -0.7071067811865475 , 0.7071067811865475 }},
        { .s= { 0.1053474006554833 , 0.8685133717566557 , 0.4843412518617614 }},
        { .s= { -0.10501865929345953 , 0.9122715296654259 , 0.3958935934845478 }},
        { .s= { 0.5391638660171921 , -0.5391638660171921 , 0.6469966392206304 }},
        { .s= { -0.5698152235643941 , -0.10628823928135941 , 0.8148701867075074 }},
        { .s= { -0.2934701936702925 , -0.9506899840249583 , 0.10031849132510615 }},
        { .s= { -0.395893593484548 , 0.9122715296654258 , 0.10501865929345931 }},
        { .s= { -0.39589359348454756 , -0.912271529665426 , 0.10501865929345933 }},
        { .s= { 0.4082482904638632 , -0.816496580927726 , 0.408248290463863 }},
        { .s= { -0.9305184620712355 , -0.20501725355122344 , 0.30348528381273476 }},
        { .s= { -0.31030659960478985 , 0.8582739368522556 , 0.40874890037998013 }},
        { .s= { 0.4046150445963541 , -0.890320034496634 , 0.20884659887152338 }},
        { .s= { -0.42183873564069446 , -0.7359241010615861 , 0.5295921058604979 }},
        { .s= { 0.426401432711221 , -0.6396021490668312 , 0.6396021490668312 }},
        { .s= { 0.29347019367029253 , 0.9506899840249584 , 0.10031849132510615 }},
        { .s= { 0.8989750671491783 , 0.30971263184130077 , 0.3097126318413007 }},
        { .s= { 0.3103065996047895 , 0.8582739368522557 , 0.40874890037998013 }},
        { .s= { 6.803773075690051e-17 , -0.5555702330196022 , 0.8314696123025452 }},
        { .s= { -0.9751174407639491 , -0.1979147287015085 , 0.09987861072809527 }},
        { .s= { -0.9122715296654259 , 0.10501865929345915 , 0.3958935934845478 }},
        { .s= { 0.8685133717566558 , -0.48434125186176136 , 0.10534740065548351 }},
        { .s= { -0.3826834323650897 , -7.029780607989446e-17 , 0.9238795325112867 }},
        { .s= { 0.31245971410378265 , -0.8095113394900795 , 0.49705162538629694 }},
        { .s= { -0.9751174407639492 , -0.09987861072809545 , 0.1979147287015083 }},
        { .s= { -0.19875685341551333 , -0.19875685341551338 , 0.9596829822606673 }},
        { .s= { 0.5773502691896257 , -0.21132486540518708 , 0.7886751345948129 }},
        { .s= { -0.40824829046386324 , 0.816496580927726 , 0.408248290463863 }},
        { .s= { 0.8095113394900796 , -0.3124597141037824 , 0.49705162538629694 }},
        { .s= { -0.7071067811865475 , -1.2989340843532398e-16 , 0.7071067811865475 }},
        { .s= { -0.2050172535512235 , 0.9305184620712355 , 0.30348528381273476 }},
        { .s= { 0.7359241010615861 , -0.5295921058604978 , 0.4218387356406945 }},
        { .s= { -0.9751174407639492 , 0.0998786107280951 , 0.1979147287015083 }},
        { .s= { 0.890320034496634 , -0.20884659887152332 , 0.40461504459635395 }},
        { .s= { 0.4934670583387375 , 0.21114877843811342 , 0.843745492253799 }},
        { .s= { 0.21114877843811342 , -0.49346705833873755 , 0.8437454922537989 }},
        { .s= { -0.9902528527693809 , -0.09848676962440892 , 0.09848676962440873 }},
        { .s= { -0.42640143271122105 , 0.6396021490668312 , 0.6396021490668312 }},
        { .s= { -0.8685133717566557 , -0.10534740065548365 , 0.4843412518617614 }},
        { .s= { -0.09987861072809551 , 0.9751174407639492 , 0.1979147287015083 }},
        { .s= { 2.389167384016779e-17 , -0.19509032201612825 , 0.9807852804032304 }},
        { .s= { 0.10657418966918712 , 0.7543444794845716 , 0.6477702898153842 }},
        { .s= { 0.10031849132510597 , 0.9506899840249583 , 0.29347019367029265 }},
        { .s= { -0.9807852804032304 , -1.8016733314449831e-16 , 0.19509032201612825 }},
        { .s= { 0.19875685341551325 , 0.9596829822606674 , 0.19875685341551338 }},
        { .s= { 0.31245971410378254 , -0.49705162538629694 , 0.8095113394900795 }},
        { .s= { 0.6907683581691451 , -0.21372447380823673 , 0.690768358169145 }},
        { .s= { -0.9506899840249583 , 0.10031849132510597 , 0.29347019367029265 }},
        { .s= { -0.4046150445963542 , 0.890320034496634 , 0.20884659887152338 }},
        { .s= { 0.10628823928135944 , -0.8148701867075075 , 0.5698152235643941 }},
        { .s= { -0.735924101061586 , -0.4218387356406948 , 0.5295921058604978 }},
        { .s= { -0.9305184620712355 , -0.303485283812735 , 0.20501725355122327 }},
        { .s= { 0.19875685341551333 , 0.19875685341551338 , 0.9596829822606674 }},
        { .s= { 0.7543444794845715 , 0.10657418966918733 , 0.6477702898153842 }},
        { .s= { -0.19791472870150834 , 0.09987861072809524 , 0.9751174407639492 }},
        { .s= { 0.9305184620712355 , -0.2050172535512232 , 0.30348528381273476 }},
        { .s= { -0.8582739368522556 , -0.3103065996047898 , 0.40874890037998013 }},
        { .s= { 0.9506899840249583 , -0.2934701936702925 , 0.10031849132510615 }},
        { .s= { 0.4843412518617614 , 0.10534740065548351 , 0.8685133717566557 }},
        { .s= { -0.21114877843811358 , 0.8437454922537989 , 0.49346705833873755 }},
        { .s= { 7.769077048515857e-17 , -0.6343932841636454 , 0.773010453362737 }},
        { .s= { -0.09987861072809516 , -0.9751174407639492 , 0.1979147287015083 }},
        { .s= { 9.466647774181201e-17 , -0.773010453362737 , 0.6343932841636454 }},
        { .s= { -0.5555702330196022 , -1.0205659613535074e-16 , 0.8314696123025452 }},
        { .s= { 0.3124597141037823 , 0.497051625386297 , 0.8095113394900796 }},
        { .s= { -9.373040810652596e-17 , 0.3826834323650897 , 0.9238795325112867 }},
        { .s= { 0.8685133717566557 , 0.10534740065548354 , 0.4843412518617614 }},
        { .s= { -0.7543444794845717 , 0.6477702898153841 , 0.10657418966918729 }},
        { .s= { 0.7206866372437452 , -0.614055377083669 , 0.321786831260907 }},
        { .s= { -0.881921264348355 , -1.6200630802262996e-16 , 0.47139673682599764 }},
        { .s= { -0.6140553770836692 , -0.3217868312609072 , 0.720686637243745 }},
        { .s= { -0.10501865929345941 , 0.3958935934845478 , 0.9122715296654259 }},
        { .s= { -0.8903200344966341 , 0.4046150445963538 , 0.2088465988715234 }},
        { .s= { -0.8164965809277261 , 0.40824829046386285 , 0.408248290463863 }},
        { .s= { 0.8095113394900796 , -0.4970516253862969 , 0.3124597141037825 }},
        { .s= { -0.5391638660171922 , 0.539163866017192 , 0.6469966392206304 }},
        { .s= { -7.109924016823996e-17 , 0.29028467725446233 , 0.9569403357322088 }},
        { .s= { 0.9122715296654259 , 0.10501865929345937 , 0.3958935934845478 }},
        { .s= { 0.4264014327112208 , 0.6396021490668313 , 0.6396021490668313 }},
        { .s= { 0.816496580927726 , 0.4082482904638631 , 0.408248290463863 }},
        { .s= { 1.0800420534841997e-16 , -0.881921264348355 , 0.47139673682599764 }},
        { .s= { -0.8685133717566557 , 0.10534740065548331 , 0.4843412518617614 }},
        { .s= { 0.3958935934845479 , -0.9122715296654258 , 0.10501865929345931 }},
        { .s= { 0.5773502691896258 , -0.7886751345948128 , 0.2113248654051871 }},
        { .s= { -0.6907683581691451 , 0.21372447380823667 , 0.690768358169145 }},
        { .s= { -0.09801714032956059 , -1.8005456574926e-17 , 0.9951847266721968 }},
        { .s= { 0.735924101061586 , 0.4218387356406947 , 0.5295921058604979 }},
        { .s= { 0.8095113394900795 , 0.3124597141037826 , 0.497051625386297 }},
        { .s= { 0.6907683581691448 , 0.2137244738082368 , 0.6907683581691451 }},
        { .s= { -0.20884659887152324 , -0.890320034496634 , 0.40461504459635395 }},
        { .s= { -0.8148701867075073 , -0.5698152235643942 , 0.1062882392813593 }},
        { .s= { 0.9238795325112867 , 5.657130561438501e-17 , 0.3826834323650897 }},
        { .s= { 0.09987861072809523 , 0.1979147287015083 , 0.9751174407639492 }},
        { .s= { -0.890320034496634 , 0.2088465988715232 , 0.40461504459635395 }},
        { .s= { 0.9596829822606673 , 0.19875685341551347 , 0.19875685341551336 }},
        { .s= { -0.6477702898153844 , 0.7543444794845714 , 0.10657418966918727 }},
        { .s= { 0.8685133717566556 , 0.4843412518617615 , 0.10534740065548348 }},
        { .s= { 0.3958935934845478 , -0.1050186592934593 , 0.9122715296654259 }},
        { .s= { -0.10657418966918746 , 0.7543444794845715 , 0.6477702898153842 }},
        { .s= { -0.40824829046386296 , -0.4082482904638631 , 0.816496580927726 }},
        { .s= { -0.2088465988715235 , 0.40461504459635395 , 0.890320034496634 }},
        { .s= { -0.9122715296654258 , -0.395893593484548 , 0.10501865929345931 }},
        { .s= { 0.6907683581691452 , -0.690768358169145 , 0.21372447380823678 }},
        { .s= { 0.9596829822606674 , -0.19875685341551327 , 0.19875685341551338 }},
        { .s= { -0.10501865929345919 , -0.9122715296654259 , 0.3958935934845478 }},
        { .s= { 0.10031849132510627 , -0.9506899840249583 , 0.29347019367029265 }},
        { .s= { -0.788675134594813 , 0.21132486540518697 , 0.5773502691896257 }},
        { .s= { -0.9596829822606674 , 0.1987568534155132 , 0.19875685341551338 }},
        { .s= { -0.7359241010615859 , -0.529592105860498 , 0.4218387356406945 }},
        { .s= { -0.3034852838127348 , 0.20501725355122322 , 0.9305184620712355 }},
        { .s= { 0.9807852804032304 , 6.005577771483278e-17 , 0.19509032201612825 }},
        { .s= { 0.8685133717566557 , -0.10534740065548343 , 0.4843412518617614 }},
        { .s= { -0.20501725355122333 , 0.30348528381273476 , 0.9305184620712355 }},
        { .s= { -0.773010453362737 , -1.4199971661271801e-16 , 0.6343932841636454 }},
        { .s= { 0.1979147287015083 , 0.09987861072809529 , 0.9751174407639492 }},
        { .s= { 0.6343932841636454 , 3.8845385242579287e-17 , 0.773010453362737 }},
        { .s= { -0.6477702898153842 , -0.1065741896691874 , 0.7543444794845715 }},
        { .s= { -0.7886751345948129 , -0.21132486540518725 , 0.5773502691896257 }},
        { .s= { -0.8582739368522556 , -0.40874890037998035 , 0.3103065996047896 }},
        { .s= { 0.32178683126090696 , 0.7206866372437452 , 0.6140553770836691 }},
        { .s= { -2.0365131985892053e-16 , 0.8314696123025452 , 0.5555702330196022 }},
        { .s= { 0.09801714032956059 , 6.001818858308667e-18 , 0.9951847266721968 }},
        { .s= { 0.39589359348454767 , 0.10501865929345934 , 0.9122715296654259 }},
        { .s= { 0.8437454922537989 , 0.21114877843811342 , 0.49346705833873755 }},
        { .s= { 0.2113248654051872 , -0.5773502691896257 , 0.7886751345948129 }},
        { .s= { -1.3607546151380102e-16 , 0.5555702330196022 , 0.8314696123025452 }},
        { .s= { -2.437499580158879e-16 , 0.9951847266721968 , 0.09801714032956059 }},
        { .s= { 0.720686637243745 , 0.3217868312609071 , 0.6140553770836692 }},
        { .s= { 0.7886751345948129 , 0.5773502691896257 , 0.2113248654051871 }},
        { .s= { 0.3103065996047898 , -0.8582739368522556 , 0.40874890037998013 }},
        { .s= { -0.9238795325112867 , -1.6971391684315505e-16 , 0.3826834323650897 }},
        { .s= { 0.40461504459635395 , -0.20884659887152338 , 0.890320034496634 }},
        { .s= { 0.5698152235643942 , -0.8148701867075073 , 0.1062882392813593 }},
        { .s= { 0.9951847266721968 , 6.093748950397197e-17 , 0.09801714032956059 }},
        { .s= { 0.0998786107280954 , -0.9751174407639492 , 0.1979147287015083 }},
        { .s= { 0.09848676962440871 , 0.09848676962440873 , 0.9902528527693809 }},
        { .s= { 4.686520405326298e-17 , -0.3826834323650897 , 0.9238795325112867 }},
        { .s= { 0.529592105860498 , -0.7359241010615859 , 0.4218387356406945 }},
        { .s= { 0.2050172535512231 , 0.9305184620712355 , 0.30348528381273476 }},
        { .s= { -0.9305184620712356 , 0.3034852838127346 , 0.2050172535512233 }},
        { .s= { 0.735924101061586 , 0.5295921058604979 , 0.42183873564069463 }},
        { .s= { -0.8148701867075077 , 0.5698152235643938 , 0.1062882392813593 }},
        { .s= { 0.40874890037998 , 0.8582739368522557 , 0.3103065996047897 }},
        { .s= { 1.2187497900794394e-16 , -0.9951847266721968 , 0.09801714032956059 }},
        { .s= { 0.3217868312609072 , -0.6140553770836692 , 0.720686637243745 }},
        { .s= { -0.10501865929345926 , -0.3958935934845478 , 0.9122715296654259 }},
        { .s= { 0.539163866017192 , 0.5391638660171921 , 0.6469966392206306 }},
        { .s= { -0.8095113394900795 , -0.4970516253862971 , 0.3124597141037825 }},
        { .s= { 0.8903200344966341 , -0.4046150445963539 , 0.2088465988715234 }},
        { .s= { -0.9596829822606673 , -0.19875685341551355 , 0.19875685341551336 }},
        { .s= { -0.8314696123025452 , -1.527384898941904e-16 , 0.5555702330196022 }},
        { .s= { 0.6396021490668314 , -0.6396021490668312 , 0.42640143271122083 }},
        { .s= { -0.30348528381273476 , -0.2050172535512233 , 0.9305184620712355 }},
        { .s= { 0.8164965809277261 , -0.40824829046386296 , 0.408248290463863 }},
        { .s= { 0.8148701867075075 , -0.10628823928135925 , 0.5698152235643941 }},
        { .s= { 0.09848676962440853 , 0.9902528527693809 , 0.09848676962440873 }},
        { .s= { -0.529592105860498 , 0.7359241010615859 , 0.4218387356406945 }},
        { .s= { -0.09987861072809533 , 0.1979147287015083 , 0.9751174407639492 }},
        { .s= { -0.10628823928135947 , 0.5698152235643941 , 0.8148701867075074 }},
        { .s= { 0.1053474006554836 , -0.8685133717566557 , 0.4843412518617614 }},
        { .s= { -0.912271529665426 , 0.3958935934845475 , 0.10501865929345933 }},
        { .s= { -0.9506899840249583 , 0.2934701936702924 , 0.10031849132510615 }},
        { .s= { -0.7886751345948128 , -0.5773502691896258 , 0.2113248654051871 }},
        { .s= { -0.20501725355122313 , -0.9305184620712355 , 0.30348528381273476 }},
        { .s= { -0.9751174407639492 , 0.19791472870150814 , 0.09987861072809529 }},
        { .s= { -0.6396021490668314 , 0.4264014327112208 , 0.6396021490668312 }},
        { .s= { -0.30971263184130077 , 0.30971263184130066 , 0.8989750671491783 }},
        { .s= { 0.6907683581691448 , 0.6907683581691451 , 0.21372447380823678 }},
        { .s= { -0.3958935934845478 , 0.10501865929345924 , 0.9122715296654259 }},
        { .s= { -0.5773502691896257 , -0.21132486540518722 , 0.7886751345948129 }},
        { .s= { -0.5698152235643938 , -0.8148701867075077 , 0.1062882392813593 }},
        { .s= { 0.31030659960478973 , -0.4087489003799802 , 0.8582739368522557 }},
        { .s= { 3.554962008411998e-17 , -0.29028467725446233 , 0.9569403357322088 }},
        { .s= { -0.42640143271122083 , -0.6396021490668314 , 0.6396021490668313 }},
        { .s= { -1.8933295548362402e-16 , 0.773010453362737 , 0.6343932841636454 }},
        { .s= { -0.539163866017192 , -0.5391638660171922 , 0.6469966392206306 }},
        { .s= { -0.49346705833873783 , 0.8437454922537988 , 0.21114877843811336 }},
        { .s= { 0.8582739368522557 , 0.31030659960478973 , 0.4087489003799802 }},
        { .s= { 0.10534740065548336 , 0.4843412518617614 , 0.8685133717566557 }},
        { .s= { -2.2628522245754005e-16 , 0.9238795325112867 , 0.3826834323650897 }},
        { .s= { -0.3103065996047896 , -0.4087489003799802 , 0.8582739368522557 }},
        { .s= { -0.8582739368522557 , 0.31030659960478946 , 0.40874890037998013 }},
        { .s= { -0.690768358169145 , -0.2137244738082369 , 0.690768358169145 }},
        { .s= { -0.21132486540518705 , -0.5773502691896258 , 0.7886751345948129 }},
        { .s= { 0.19791472870150847 , -0.9751174407639492 , 0.09987861072809529 }},
        { .s= { 0.9506899840249583 , -0.10031849132510606 , 0.29347019367029265 }},
        { .s= { 1.2003637716617333e-17 , -0.09801714032956059 , 0.9951847266721968 }},
        { .s= { -0.10031849132510602 , -0.9506899840249583 , 0.29347019367029265 }},
        { .s= { -0.29347019367029265 , 0.10031849132510609 , 0.9506899840249583 }},
        { .s= { -0.48434125186176163 , 0.8685133717566556 , 0.10534740065548347 }},
        { .s= { 0.10501865929345944 , -0.9122715296654259 , 0.3958935934845478 }},
        { .s= { -2.3438278382588863e-16 , 0.9569403357322088 , 0.29028467725446233 }},
        { .s= { 0.7359241010615861 , -0.4218387356406945 , 0.5295921058604978 }},
        { .s= { 0.6477702898153842 , 0.7543444794845715 , 0.10657418966918727 }},
        { .s= { -0.19875685341551358 , 0.9596829822606673 , 0.19875685341551336 }},
        { .s= { -0.1065741896691872 , -0.6477702898153842 , 0.7543444794845715 }},
        { .s= { -0.7543444794845716 , 0.10657418966918714 , 0.6477702898153842 }},
        { .s= { -0.5698152235643941 , 0.10628823928135923 , 0.8148701867075075 }},
        { .s= { 0.6469966392206304 , 0.5391638660171921 , 0.5391638660171921 }},
        { .s= { 0.20501725355122338 , -0.9305184620712355 , 0.30348528381273476 }},
        { .s= { -0.8095113394900795 , -0.31245971410378265 , 0.49705162538629694 }},
        { .s= { -0.19875685341551325 , -0.9596829822606674 , 0.19875685341551338 }},
        { .s= { -0.40824829046386285 , -0.8164965809277261 , 0.408248290463863 }},
        { .s= { 0.9305184620712355 , 0.3034852838127349 , 0.20501725355122327 }},
        { .s= { -0.8148701867075075 , -0.10628823928135947 , 0.5698152235643941 }},
        { .s= { 0.6477702898153844 , -0.7543444794845714 , 0.10657418966918727 }},
        { .s= { 0.539163866017192 , 0.6469966392206306 , 0.5391638660171921 }},
        { .s= { -0.10031849132510637 , 0.9506899840249583 , 0.29347019367029265 }},
        { .s= { -0.8148701867075075 , 0.10628823928135915 , 0.5698152235643941 }},
        { .s= { -0.6396021490668312 , -0.426401432711221 , 0.6396021490668312 }},
        { .s= { 1.2011155542966555e-16 , -0.9807852804032304 , 0.19509032201612825 }},
        { .s= { 0.49346705833873744 , 0.843745492253799 , 0.2111487784381134 }},
        { .s= { 0.3958935934845476 , 0.912271529665426 , 0.10501865929345933 }},
        { .s= { 0.4970516253862968 , 0.8095113394900796 , 0.31245971410378254 }},
        { .s= { 0.20884659887152351 , -0.890320034496634 , 0.40461504459635395 }},
        { .s= { 0.10628823928135937 , -0.5698152235643941 , 0.8148701867075074 }},
        { .s= { 0.2111487784381135 , -0.8437454922537989 , 0.49346705833873755 }},
        { .s= { -1.1545890097649308e-16 , 0.47139673682599764 , 0.881921264348355 }},
        { .s= { 0.8989750671491784 , -0.3097126318413007 , 0.30971263184130077 }},
        { .s= { 0.40874890037998 , 0.3103065996047897 , 0.8582739368522557 }},
        { .s= { 0.9751174407639492 , -0.0998786107280952 , 0.1979147287015083 }},
        { .s= { -0.2111487784381135 , 0.49346705833873755 , 0.8437454922537989 }},
        { .s= { -0.40874890037998035 , 0.8582739368522556 , 0.3103065996047896 }},
        { .s= { 0.20884659887152343 , -0.40461504459635395 , 0.890320034496634 }},
        { .s= { -4.778334768033558e-17 , 0.19509032201612825 , 0.9807852804032304 }},
        { .s= { 0.4970516253862968 , 0.31245971410378254 , 0.8095113394900796 }},
        { .s= { 0.8582739368522557 , 0.40874890037998024 , 0.3103065996047897 }},
        { .s= { -0.4218387356406948 , 0.735924101061586 , 0.5295921058604978 }},
        { .s= { -0.7359241010615861 , 0.4218387356406944 , 0.5295921058604978 }},
        { .s= { 0.3097126318413007 , -0.3097126318413007 , 0.8989750671491783 }},
        { .s= { -0.5295921058604978 , -0.4218387356406947 , 0.735924101061586 }},
        { .s= { 0.9305184620712356 , -0.3034852838127347 , 0.2050172535512233 }},
        { .s= { 0.5295921058604979 , -0.42183873564069463 , 0.735924101061586 }},
        { .s= { -0.7543444794845715 , -0.10657418966918741 , 0.6477702898153842 }},
        { .s= { -0.09987861072809524 , -0.1979147287015083 , 0.9751174407639492 }},
        { .s= { 0.10657418966918736 , -0.6477702898153842 , 0.7543444794845715 }},
        { .s= { 0.30971263184130066 , 0.30971263184130077 , 0.8989750671491784 }},
        { .s= { -0.32178683126090696 , -0.7206866372437452 , 0.6140553770836691 }},
        { .s= { -0.48434125186176147 , 0.1053474006554834 , 0.8685133717566557 }},
        { .s= { 0.48434125186176163 , -0.8685133717566556 , 0.10534740065548348 }},
        { .s= { -0.8989750671491783 , -0.30971263184130093 , 0.3097126318413007 }},
        { .s= { 0.5773502691896256 , 0.21132486540518716 , 0.788675134594813 }},
        { .s= { -0.720686637243745 , -0.3217868312609072 , 0.6140553770836692 }},
        { .s= { 0.10657418966918739 , -0.7543444794845715 , 0.6477702898153842 }},
        { .s= { 0.5295921058604977 , 0.42183873564069463 , 0.7359241010615861 }},
        { .s= { -0.09848676962440898 , 0.9902528527693809 , 0.09848676962440873 }},
        { .s= { 0.4934670583387378 , -0.8437454922537989 , 0.21114877843811336 }},
        { .s= { -0.4970516253862968 , -0.8095113394900796 , 0.31245971410378254 }},
        { .s= { 0.2050172535512232 , 0.30348528381273476 , 0.9305184620712355 }},
        { .s= { 0.21132486540518697 , 0.788675134594813 , 0.5773502691896257 }},
        { .s= { 0.49346705833873755 , -0.21114877843811336 , 0.8437454922537989 }},
        { .s= { -0.30971263184130066 , -0.3097126318413007 , 0.8989750671491783 }},
        { .s= { -0.10534740065548336 , -0.8685133717566557 , 0.4843412518617614 }},
        { .s= { -0.10628823928135926 , -0.5698152235643941 , 0.8148701867075075 }},
        { .s= { -0.6469966392206307 , 0.539163866017192 , 0.539163866017192 }},
        { .s= { 0.7071067811865475 , 4.3297802811774664e-17 , 0.7071067811865475 }},
        { .s= { -0.788675134594813 , 0.5773502691896255 , 0.2113248654051871 }},
        { .s= { -0.09848676962440876 , 0.09848676962440872 , 0.9902528527693809 }},
        { .s= { 0.890320034496634 , 0.20884659887152343 , 0.40461504459635395 }},
        { .s= { 0.3217868312609072 , -0.720686637243745 , 0.6140553770836692 }},
        { .s= { 0.48434125186176136 , 0.8685133717566558 , 0.10534740065548351 }},
        { .s= { 1.0182565992946027e-16 , -0.8314696123025452 , 0.5555702330196022 }},
        { .s= { -0.5391638660171922 , 0.6469966392206304 , 0.539163866017192 }},
        { .s= { -0.1979147287015083 , -0.0998786107280953 , 0.9751174407639492 }},
        { .s= { 0.2111487784381132 , 0.843745492253799 , 0.49346705833873755 }},
        { .s= { 0.6140553770836693 , -0.720686637243745 , 0.321786831260907 }},
        { .s= { 0.4082482904638629 , 0.4082482904638631 , 0.8164965809277261 }},
        { .s= { -2.4007275433234667e-17 , 0.09801714032956059 , 0.9951847266721968 }},
        { .s= { 0.9902528527693809 , -0.09848676962440865 , 0.09848676962440873 }},
        { .s= { 0.5295921058604977 , 0.7359241010615861 , 0.42183873564069463 }},
        { .s= { -2.402231108593311e-16 , 0.9807852804032304 , 0.19509032201612825 }},
        { .s= { 0.3103065996047895 , 0.40874890037998013 , 0.8582739368522557 }},
        { .s= { 0.8095113394900795 , 0.497051625386297 , 0.3124597141037825 }},
        { .s= { -0.7359241010615861 , 0.5295921058604977 , 0.4218387356406945 }},
        { .s= { 0.29347019367029276 , -0.9506899840249583 , 0.10031849132510615 }},
        { .s= { -0.3124597141037825 , -0.49705162538629705 , 0.8095113394900795 }},
        { .s= { -0.4934670583387375 , -0.8437454922537991 , 0.21114877843811336 }},
        { .s= { -0.5391638660171919 , -0.6469966392206306 , 0.539163866017192 }},
        { .s= { -0.19791472870150853 , 0.9751174407639491 , 0.09987861072809527 }},
        { .s= { -0.890320034496634 , -0.4046150445963542 , 0.20884659887152338 }},
        { .s= { -0.8437454922537989 , -0.21114877843811353 , 0.49346705833873755 }},
        { .s= { -0.9305184620712355 , 0.2050172535512231 , 0.30348528381273476 }},
        { .s= { -0.890320034496634 , -0.20884659887152354 , 0.40461504459635395 }},
        { .s= { -0.10628823928135918 , -0.8148701867075075 , 0.5698152235643941 }},
        { .s= { 0.6396021490668312 , 0.6396021490668313 , 0.4264014327112209 }},
        { .s= { 0.8314696123025452 , 5.0912829964730134e-17 , 0.5555702330196022 }},
        { .s= { -0.10628823928135951 , 0.8148701867075075 , 0.5698152235643941 }},
        { .s= { 0.5391638660171922 , -0.6469966392206304 , 0.539163866017192 }},
        { .s= { 0.10534740065548354 , -0.4843412518617614 , 0.8685133717566557 }},
        { .s= { -0.32178683126090724 , 0.6140553770836692 , 0.720686637243745 }},
        { .s= { 0.4087489003799802 , -0.3103065996047897 , 0.8582739368522557 }},
        { .s= { -0.31245971410378265 , 0.8095113394900795 , 0.49705162538629694 }},
        { .s= { -0.9569403357322088 , -1.7578708786941646e-16 , 0.29028467725446233 }},
        { .s= { 0.4218387356406948 , -0.735924101061586 , 0.5295921058604978 }},
        { .s= { -0.1053474006554836 , 0.4843412518617614 , 0.8685133717566557 }},
        { .s= { 0.30348528381273476 , -0.20501725355122327 , 0.9305184620712355 }},
        { .s= { 0.773010453362737 , 4.7333238870906005e-17 , 0.6343932841636454 }},
        { .s= { 0.21114877843811325 , 0.49346705833873755 , 0.843745492253799 }},
        { .s= { 1.1719139191294432e-16 , -0.9569403357322088 , 0.29028467725446233 }},
        { .s= { -0.21132486540518727 , 0.5773502691896257 , 0.7886751345948129 }},
        { .s= { 0.6140553770836692 , -0.32178683126090707 , 0.720686637243745 }},
        { .s= { -2.1600841069683993e-16 , 0.881921264348355 , 0.47139673682599764 }},
        { .s= { 0.10501865929345937 , -0.3958935934845478 , 0.9122715296654259 }},
        { .s= { 0.19509032201612825 , 1.1945836920083895e-17 , 0.9807852804032304 }},
        { .s= { 0.2050172535512233 , -0.30348528381273476 , 0.9305184620712355 }},
        { .s= { 0.881921264348355 , 5.4002102674209984e-17 , 0.47139673682599764 }},
        { .s= { 0.3097126318413009 , -0.8989750671491783 , 0.3097126318413007 }},
        { .s= { -0.9506899840249582 , -0.2934701936702928 , 0.10031849132510613 }},
        { .s= { -0.720686637243745 , -0.6140553770836693 , 0.321786831260907 }},
        { .s= { 0.21372447380823686 , -0.690768358169145 , 0.690768358169145 }},
        { .s= { 0.3124597141037823 , 0.8095113394900796 , 0.497051625386297 }},
        { .s= { 0.843745492253799 , -0.4934670583387375 , 0.21114877843811336 }},
        { .s= { -0.21114877843811322 , -0.843745492253799 , 0.49346705833873755 }},
        { .s= { 0.788675134594813 , -0.21132486540518705 , 0.5773502691896257 }},
        { .s= { -0.8685133717566556 , -0.48434125186176163 , 0.10534740065548347 }},
        { .s= { 0.9506899840249583 , 0.29347019367029276 , 0.10031849132510615 }},
        { .s= { 0.8148701867075077 , -0.5698152235643938 , 0.1062882392813593 }},
        { .s= { -0.3097126318413006 , -0.8989750671491784 , 0.30971263184130077 }},
        { .s= { 0.8582739368522557 , -0.40874890037998 , 0.3103065996047897 }},
        { .s= { 0.40461504459635383 , 0.8903200344966341 , 0.2088465988715234 }},
        { .s= { -0.614055377083669 , -0.7206866372437453 , 0.321786831260907 }},
        { .s= { -0.5773502691896255 , -0.788675134594813 , 0.2113248654051871 }},
        { .s= { 0.4046150445963539 , 0.2088465988715234 , 0.890320034496634 }},
        { .s= { 0.614055377083669 , 0.3217868312609071 , 0.7206866372437452 }},
        { .s= { -0.4843412518617614 , -0.10534740065548356 , 0.8685133717566557 }},
        { .s= { 0.2088465988715232 , 0.890320034496634 , 0.40461504459635395 }},
        { .s= { 0.0998786107280953 , -0.1979147287015083 , 0.9751174407639492 }},
        { .s= { 0.19875685341551338 , -0.19875685341551336 , 0.9596829822606673 }},
        { .s= { 0.10501865929345922 , 0.3958935934845478 , 0.9122715296654259 }},
        { .s= { -0.6396021490668314 , 0.6396021490668312 , 0.42640143271122083 }},
        { .s= { -0.10534740065548369 , 0.8685133717566557 , 0.4843412518617614 }},
        { .s= { -0.4218387356406945 , -0.5295921058604979 , 0.735924101061586 }},
        { .s= { -0.6343932841636454 , -1.1653615572773784e-16 , 0.773010453362737 }},
        { .s= { 0.3034852838127347 , 0.20501725355122327 , 0.9305184620712355 }},
        { .s= { 0.5555702330196022 , 3.4018865378450254e-17 , 0.8314696123025452 }},
        { .s= { -0.6469966392206304 , -0.5391638660171922 , 0.539163866017192 }},
        { .s= { -0.3124597141037823 , -0.8095113394900796 , 0.497051625386297 }},
        { .s= { 0.9751174407639492 , 0.09987861072809535 , 0.1979147287015083 }},
        { .s= { 0.42183873564069446 , 0.7359241010615861 , 0.5295921058604979 }},
        { .s= { -0.40461504459635383 , -0.8903200344966341 , 0.2088465988715234 }},
        { .s= { 0.6396021490668312 , 0.42640143271122094 , 0.6396021490668313 }},
        { .s= { -0.5295921058604977 , -0.7359241010615861 , 0.4218387356406945 }},
        { .s= { 0.788675134594813 , -0.5773502691896256 , 0.2113248654051871 }},
        { .s= { 0.9751174407639492 , -0.1979147287015082 , 0.09987861072809529 }},
        { .s= { 0.4082482904638629 , 0.8164965809277261 , 0.4082482904638631 }},
        { .s= { -0.30348528381273504 , 0.9305184620712355 , 0.20501725355122327 }},
        { .s= { -0.19509032201612825 , -3.583751076025168e-17 , 0.9807852804032304 }},
        { .s= { -0.49705162538629694 , -0.3124597141037826 , 0.8095113394900795 }},
        { .s= { 0.47139673682599764 , 2.886472524412327e-17 , 0.881921264348355 }},
        { .s= { 0.9122715296654259 , -0.10501865929345926 , 0.3958935934845478 }},
        { .s= { -0.09848676962440861 , -0.9902528527693809 , 0.09848676962440873 }},
        { .s= { -0.6140553770836693 , 0.32178683126090696 , 0.720686637243745 }},
        { .s= { 0.4218387356406947 , -0.5295921058604978 , 0.735924101061586 }},
        { .s= { 0.6477702898153842 , -0.10657418966918723 , 0.7543444794845715 }},
        { .s= { -0.577350269189626 , 0.7886751345948128 , 0.2113248654051871 }},
        { .s= { -0.47139673682599764 , -8.659417573236978e-17 , 0.881921264348355 }},
        { .s= { -0.31245971410378265 , 0.49705162538629694 , 0.8095113394900795 }},
        { .s= { -0.29347019367029287 , 0.9506899840249582 , 0.10031849132510613 }},
        { .s= { -0.7206866372437453 , 0.614055377083669 , 0.321786831260907 }},
        { .s= { -0.1003184913251061 , -0.29347019367029265 , 0.9506899840249583 }},
        { .s= { -0.7206866372437452 , 0.32178683126090696 , 0.6140553770836691 }},
        { .s= { -0.9506899840249583 , -0.10031849132510633 , 0.29347019367029265 }},
        { .s= { 0.42183873564069446 , 0.5295921058604979 , 0.7359241010615861 }},
    };


    static const int numDirectionalVectors = (int)(sizeof(directionalVectors)/sizeof(Vector3)) ;
    
    for (int i = 0; i < geo->numWalls; i++)
    {
        printf("processing wall %d/%d\n", i+1, geo->numWalls);

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
            
            float lightSum= 0;
            float facSum  = 0;
            for (int k = 0; k < numDirectionalVectors; k++)
            {
                float fac = directionalVectors[k].s[2]; //== dot product between the vector and (0,0,1), == cosine between surface normal and light direction
                Vector3 dir = transformToOrthoNormalBase( directionalVectors[k], b1, b2, wall->n);
                assert(fabsf(length(dir) - 1.0f) < 1E-6);
                
                Vector3 pos = getTileCenter(wall, j);
                float dist = INFINITY;
                Rectangle* target = NULL;
                int hasHit = findClosestIntersection(pos, dir, &root, &dist, 0, &target, 0);
                if (!hasHit)    //hit a window/light source
                {
                    lightSum += fac;
                    dist = 10;
                }

                distSum += /*dist / ( dist + 1 )*/dist * fac;
                facSum += fac;
                   
            }

            distSum /= (facSum*1.5);
            lightColors[wall->lightmapSetup.s[0] + j] = 
                vec3( distSum, distSum, distSum);
        }
    }
    freeBspTree(&root);
}
