
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

#if 0
//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void createBase( const Vector3 ndir, Vector3 *c1, Vector3 *c2) {
    *c1 = (Vector3)(0,0,1);
    //printf("c1: (%f, %f, %f)\n", (*c1).s0, (*c1).s1, (*c1).s2);
    if (fabs( dot(*c1, ndir)) == 1) //are colinear --> cannot build coordinate base
        *c1 = (Vector3)(0,1,0);

    //printf("c1: (%f, %f, %f)\n", (*c1).s0, (*c1).s1, (*c1).s2);
        
    *c2 = normalize( cross(*c1,ndir));
    *c1 = normalize( cross(*c2,ndir));
    /*printf("n : (%f, %f, %f)\n", ndir.s0,  ndir.s1,  ndir.s2);
    printf("c1: (%f, %f, %f)\n", (*c1).s0, (*c1).s1, (*c1).s2);
    printf("c2: (%f, %f, %f)\n", (*c2).s0, (*c2).s1, (*c2).s2);*/
}
#endif

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

    char* spaces = (char*)malloc(depth*2+1);
    for (int i = 0; i < depth*2+1; i++)
        spaces[i] = ' ';
    spaces[depth*2] = '\0';
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



static void photonmap( const Rectangle *window, const BspTreeNode* root, Vector3 *lightColors/*, const int numLightColors*/, int isWindow, int numSamples)
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

void performGlobalIlluminationNative(Geometry *geo, Vector3* lightColors, int numSamplesPerArea)
{
    BspTreeNode root = { .numItems = geo->numWalls, .left = NULL, .right=NULL };
    root.items =  (Rectangle*)malloc(sizeof(Rectangle) * geo->numWalls);
    memcpy( root.items, geo->walls, sizeof(Rectangle) * geo->numWalls);
    
    printf("root size: %d\n", root.numItems);
    
    /*for (int i = 0; i < root.numItems; i++)
    {
        BspTreeNode r2 = root;
        r2.items = (Rectangle*)malloc(sizeof(Rectangle) * root.numItems);
        memcpy( r2.items, root.items, sizeof(Rectangle) * root.numItems);

        
        subdivideNode(&r2, &(root.items[i]) );

        int l = r2.left ? r2.left->numItems : 0;
        int r = r2.right ? r2.right->numItems : 0;
        printf("node sizes (max/L/C/R): %d/%d/%d/%d\n", 
            ( l > r ? l : r)+r2.numItems,l, r2.numItems, r);
    }*/
    subdivideNode(&root, 0);

    int l = root.left ? root.left->numItems : 0;
    int r = root.right ? root.right->numItems : 0;
    printf("node sizes (max/L/C/R): %d/%d/%d/%d\n", 
        ( l > r ? l : r)+root.numItems,l, root.numItems, r);
    
    //exit(0);
    //root.plane = 


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

