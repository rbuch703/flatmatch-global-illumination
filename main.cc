
#include "rectangle.h"
#include "parseLayout.h"
#include "png_helper.h"
#include "vector3_cl.h"

#include <list>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <stdlib.h> //for rand()
//#include <string>
#include <string.h> //for memset
using namespace std;


Rectangle* objects;
cl_int numObjects;

Vector3* lightColors;
int numLightColors;

Vector3 getUniformDistributedRandomRay(Vector3 ndir, Vector3 udir, Vector3 vdir)
{
/*    //Marsaglia polar method for creating gaussian-distributed random variates
    float x = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    float y = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    float s = x*x+y*y;

    float u = x * sqrt( (-2*log(s)/s)); //has to be the natural logarithm! ( c's log() is fine)
    float v = y * sqrt( (-2*log(s)/s));

    x = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    y = (rand() / (float)RAND_MAX)*2 - 1.0; // uniform in [-1, 1]
    s = x*x+y*y;
    float n = x * sqrt( (-2*log(s)/s));    

    float len = sqrt(u*u+v*v+n*n);
    u/=len;
    v/=len;
    n/=len;*/
    
    //HACK: computes not a spherically uniform distribution, but a lambertian quarter-sphere (lower half of hemisphere)
    //# Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand() / (float)RAND_MAX);
    float phi = 2 * 3.141592 * rand() / (float)RAND_MAX;

    //# Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    if (v > 0)  //project to lower quadsphere (no light from below the horizon)
        v = -v;

    //# Convert to a direction on the hemisphere defined by the normal
    return add(add(mul(udir, u), mul(vdir, v)), mul(ndir, n));

}

Vector3 getCosineDistributedRandomRay(Vector3 ndir, Vector3 udir, Vector3 vdir) {
    //# Compute a uniformly distributed point on the unit disk
    float r = sqrt(rand() / (float)RAND_MAX);
    float phi = 2 * 3.141592 * rand() / (float)RAND_MAX;

    //# Project point onto unit hemisphere
    float u = r * cos(phi);
    float v = r * sin(phi);
    float n = sqrt(1 - r*r);

    //# Convert to a direction on the hemisphere defined by the normal
    //return udir*u + vdir*v + ndir*n;
    return add(add(mul(udir, u), mul(vdir, v)), mul(ndir, n));
    
}

//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
void createBase(Vector3 ndir, Vector3 &c1, Vector3 &c2) {
    c1 = createVector3(0,0,1);
    if (fabs(dot(c1, ndir)) == 1) //are colinear --> cannot build coordinate base
        c1 = createVector3(0,1,0);
        
    c2 = normalized( cross(c1,ndir));
    c1 = normalized( cross(c2,ndir));
}

Rectangle* getClosestObject(const Vector3 &ray_src, const Vector3 &ray_dir, Rectangle *objects, int numObjects, float &dist_out)
{
    Rectangle *closestObject = NULL;
    dist_out = INFINITY;
    
    //int numObjects = objects.size();
    
    for ( int i = 0; i < numObjects; i++)
    {
        float dist = intersects( &(objects[i]), ray_src, ray_dir, dist_out);
        if (dist < 0)
            continue;
            
        if (dist < dist_out) {
            closestObject = &(objects[i]);
            dist_out = dist;
        }
    }
    return closestObject;
}

Vector3 getObjectColorAt(Rectangle* rect, Vector3 pos)
{
    
    Vector3 light = lightColors[ rect->lightBaseIdx + getTileIdAt(rect, pos)];
    return createVector3( light.s[0] * rect->color.s[0],
                          light.s[1] * rect->color.s[1],
                          light.s[2] * rect->color.s[2]);
}

Vector3 getColor(Vector3 ray_src, Vector3 ray_dir, Rectangle* objects, int numObjects/*, int depth = 0*/)
{

    float hitDist;
    Rectangle *hitObject = getClosestObject(ray_src, ray_dir, objects, numObjects, hitDist);

    if (! hitObject)         
        return createVector3(0,0,0);

    //Vector3 intersect_pos = ray_src + ray_dir * hitDist;
    Vector3 intersect_pos = add(ray_src, mul(ray_dir, hitDist));
    return getObjectColorAt(hitObject, intersect_pos);
}


void loadGeometry()
{
    static const Vector3 wallColor = createVector3(0.8, 0.8, 0.8);
    
    vector<Rectangle> rects = parseLayout("layout.png");
    numObjects = 2 + rects.size();
    if (0 != posix_memalign((void**)&objects, 16, numObjects * sizeof(Rectangle))) return;
    //objects = (Rectangle*)malloc( numObjects * sizeof(Rectangle));
    objects[0] = createRectangleWithColor( createVector3(0,0,0), createVector3(0, 1000, 0), createVector3(1000, 0, 0), wallColor);    // floor
    objects[1] = createRectangleWithColor( createVector3(0,0,200), createVector3(1000, 0, 0), createVector3(0, 1000, 0), wallColor);  // ceiling

    int idx = 2;    
    for ( vector<Rectangle>::const_iterator it = rects.begin(); it != rects.end(); it++)
        objects[idx++] = *it;

    numLightColors = 0;
    for ( int i = 0; i < numObjects; i++)
    {
        objects[i].lightBaseIdx = numLightColors;
        numLightColors += getNumTiles(&objects[i]);
    }
    lightColors = (Vector3*) malloc( numLightColors * sizeof(Vector3));
    for (int i = 0; i < numLightColors; i++)
        lightColors[i] = createVector3(0,0,0);
}

void printDeviceString(cl_device_id dev_id)
{
    size_t res_size;
    cout << "device " << dev_id << endl;

    clGetDeviceInfo( dev_id,   CL_DEVICE_NAME , 0, NULL, &res_size);
    //cout << "\t result size is " << res_size << " bytes" << endl;
    
    char * res = new char[res_size];
    res[0] = 0;
    
    #ifndef NDEBUG
    cl_int status =
    #endif
        clGetDeviceInfo( dev_id,   CL_DEVICE_NAME  , res_size, res, NULL);
    assert(status == CL_SUCCESS);
    
    //cout << "\t status code is " << status << endl;
  	cout << "\t has name '" << res << "'" << endl;
  	delete [] res;
}

void clErrorFunc ( const char *errinfo, const void * /*private_info*/, size_t /*cb*/, void * /*user_data*/)
{
    cout << errinfo << endl;
}

void initCl(cl_context *ctx, cl_device_id *device) {
    
	cl_uint numPlatforms = 0;	//the NO. of platforms
    clGetPlatformIDs(0, NULL, &numPlatforms);
    assert( numPlatforms == 1);
    cout << "OpenCl found " << numPlatforms << " platforms" << endl;
	cl_platform_id platform_id;// = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
	clGetPlatformIDs(numPlatforms, &platform_id, NULL);

    cl_uint numDevices = 0;
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
	assert(numDevices == 1);
	//cl_device_id device_id = 0;
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, numDevices, device, NULL);
    printDeviceString( *device );
    
    
    cout << "CL_INVALID_DEVICE:  " << CL_INVALID_DEVICE << endl;
    cout << "CL_INVALID_VALUE:   " << CL_INVALID_VALUE << endl;
    cout << "CL_INVALID_PLATFORM:" << CL_INVALID_PLATFORM << endl;
    cout << "CL_DEVICE_NOT_AVAILABLE" << CL_DEVICE_NOT_AVAILABLE << endl;
    cout << "CL_DEVICE_NOT_FOUND" << CL_DEVICE_NOT_FOUND << endl;
    cout << "CL_OUT_OF_HOST_MEMORY" << CL_OUT_OF_HOST_MEMORY << endl;
    cout << "CL_INVALID_DEVICE_TYPE" << CL_INVALID_DEVICE_TYPE << endl;

    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id, 0};

    cl_int status;
    *ctx = clCreateContext(properties, numDevices, device, clErrorFunc,NULL,&status);
    assert(status == CL_SUCCESS);

}


char* getProgramString(const char* filename)
{
    char* res;
    
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    res = (char*)malloc( size+1); //plus zero-termination

    fseek(f, 0, SEEK_SET);
    size_t nRead = fread(res, size, 1, f);
    res[size] = '\0';
    fclose(f);
    
    if (nRead != 1)
    {
        free(res);
        res = NULL;
    }
    
    return res;    
}

cl_program createProgram(cl_context ctx, cl_device_id device, const char* src)
{
    cl_int status;    
	cl_program program = clCreateProgramWithSource(ctx, 1, &src, NULL, &status);
    cout << "clCreateProgramWithSource: " << status << endl;
	assert(status == CL_SUCCESS);

	status = clBuildProgram(program, 1,&device,"-g",NULL,NULL);
    cout << "clBuildProgram: " << status << endl;
	assert(status == CL_SUCCESS);
    if (status != CL_SUCCESS)
    {
        size_t size = 0;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
        char* msg = (char*)malloc(size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, size, msg, NULL);
        cout << msg << endl;
        free(msg);
        cout << "Compilation errors found, exiting ..." << endl;
        exit(0);
        //return 0;   
    }
    return program;
}

int main()
{
    cout << "Rectangle size is " << sizeof(Rectangle) << " bytes" << endl;

    loadGeometry();

    vector<Rectangle> windows;
    for ( int i = 0; i < numObjects; i++)
    {
        //Color3 col = (*it)->getColor( (*it)->getTileCenter(0) );
        Vector3 col = objects[i].color;
        if (col.s[0] > 1 || col.s[1] > 1 || col.s[2] > 1)
            windows.push_back( objects[i]);
    }
    cout << "Registered " << windows.size() << " windows" << endl;


    cl_context ctx;
    cl_device_id device;
	initCl(&ctx, &device);
	cl_command_queue queue = clCreateCommandQueue(ctx, device, 0, NULL);
	
    cout << "context: " << ctx << ", queue: " << queue << endl;
    char* program_str = getProgramString("photonmap.cl");

    cl_program prog = createProgram(ctx, device, program_str);
    free(program_str);


    cl_mem rectBuffer = clCreateBuffer(ctx, CL_MEM_READ_ONLY |CL_MEM_COPY_HOST_PTR, numObjects * sizeof(Rectangle),(void *) objects, NULL);
    cout << "rectBuffer: " << rectBuffer << endl;
    
    for ( unsigned int i = 0; i < windows.size(); i++)
    {
        Rectangle window = windows[i];
        for (int j = 0; j < getNumTiles(&window); j++)
            lightColors[ window.lightBaseIdx + j] = createVector3(1, 1, 1);
            //getTile(&window, j).setLightColor(  );

        Vector3 src = getOrigin(       &window);
        Vector3 xDir= getWidthVector(  &window);
        Vector3 yDir= getHeightVector( &window);
        
        //float area = xDir.length() * yDir.length();
        float area = length(xDir) * length(yDir);
         
        Vector3 xNorm = normalized(xDir);
        Vector3 yNorm = normalized(yDir);
        
        /* ================= */
        /* SET ACCURACY HERE */
        /* ================= */
        static const int numSamplesPerArea = 1000;
        int numSamples = numSamplesPerArea * area;

        cout << "Photon-Mapping window " << (i+1) << "/" << windows.size() << " with " << numSamples << " samples" << endl;
        Vector3 *ray_src, *ray_dir;
        if (0 != posix_memalign((void**)&ray_src, 16, numSamples * sizeof(Vector3))) return -1;
        if (0 != posix_memalign((void**)&ray_dir, 16, numSamples * sizeof(Vector3))) return -1;

        int     *hit_obj;
        if (0 != posix_memalign((void**)&hit_obj, 16, numSamples * sizeof(int) )) return -1;

        float   *hit_dist;
        if (0 != posix_memalign((void**)&hit_dist,16, numSamples * sizeof(float) )) return -1;
        
        
        

        for (int i = 0; i < numSamples; i++)
        {
            

            float dx = rand() / (float)RAND_MAX;
            float dy = rand() / (float)RAND_MAX;

            //Vector3 pos = src + xDir*dx + yDir*dy;
            Vector3 pos = add( src, add(mul(xDir, dx), mul(yDir, dy)));
            Vector3 n   = window.n;

            ray_dir[i] = getUniformDistributedRandomRay(n, xNorm, yNorm);
            hit_obj[i] = -1;
            hit_dist[i] = INFINITY;

            pos = add(pos, mul(ray_dir[i], 1E-10f)); //to prevent self-intersection on the light source geometry
            ray_src[i] = pos;
        }

	    cl_mem rayDirBuffer = clCreateBuffer(ctx, CL_MEM_READ_ONLY |CL_MEM_COPY_HOST_PTR, numSamples * sizeof(Vector3),(void *) ray_dir, NULL);
	    cl_mem raySrcBuffer = clCreateBuffer(ctx, CL_MEM_READ_ONLY |CL_MEM_COPY_HOST_PTR, numSamples * sizeof(Vector3),(void *) ray_src, NULL);
	    cl_mem hitObjBuffer = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY|CL_MEM_COPY_HOST_PTR, numSamples * sizeof(cl_int),(void *) hit_obj, NULL);
	    cl_mem hitDistBuffer= clCreateBuffer(ctx, CL_MEM_WRITE_ONLY|CL_MEM_COPY_HOST_PTR, numSamples * sizeof(float),(void *) hit_dist, NULL);

        cl_int status = 0;
        cl_kernel kernel = clCreateKernel(prog,"photonmap", &status);
        cout << "clCreateKernel: " << status << endl;
        /*Step 9: Sets Kernel arguments.*/
        status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&raySrcBuffer);
        status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&rayDirBuffer);
        status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&rectBuffer);
        status = clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&numObjects);
        status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&hitObjBuffer);
        status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&hitDistBuffer);

        size_t workSize = numSamples;
    	status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &workSize, NULL, 0, NULL, NULL);
    	cout << "clEnqueueNDRangeKernel: " << status << endl;
        clFinish(queue);
        
	    cout << "kernel processing complete, exiting ..." << endl;

        clEnqueueReadBuffer(queue, hitObjBuffer, CL_TRUE, 0, numSamples * sizeof(cl_int),  (void *) hit_obj, 0, NULL, NULL);
        clEnqueueReadBuffer(queue, hitDistBuffer, CL_TRUE, 0, numSamples * sizeof(float),  (void *) hit_dist, 0, NULL, NULL);
        
        clReleaseMemObject(rayDirBuffer);
        clReleaseMemObject(raySrcBuffer);
        clReleaseMemObject(hitObjBuffer);
        clReleaseMemObject(hitDistBuffer);

        for (int i = 0; i < numSamples; i++)
        {
            Vector3 lightCol = window.color;
            //cout << "hit_obj[" << i << "] = " << hit_obj[i] << endl;
            if (hit_obj[i] == -1) continue;
            
            Rectangle *hit = &objects[ hit_obj[i]];
            Vector3 hitPos = add(ray_src[i], mul(ray_dir[i], hit_dist[i]));

            Vector3 tileCol = hit->color;
            if (tileCol.s[0] > 1 || tileCol.s[1] > 1 || tileCol.s[2] > 1)    //hit a light source
                continue;

            lightColors[ hit->lightBaseIdx + getTileIdAt(hit, hitPos)] = 
                add(lightColors[ hit->lightBaseIdx + getTileIdAt(hit, hitPos)],
                    mul(lightCol, (1 / (TILE_SIZE*TILE_SIZE*numSamplesPerArea*2))));
            
        }
	    //exit(0);  

            //Vector3 lightCol = window.color;
            #if 0
            for (int depth = 0; depth < 1; depth++)
            {
                
                float dist;
                Rectangle *hit = getClosestObject(pos, ray_dir, objects, numObjects, dist);
                if (!hit) continue;
                
                Vector3 hitPos = add(pos, mul(ray_dir, dist));
                //Tile& tile = getTileAt(hit, hitPos);
                Vector3 tileCol = hit->color;
                if (tileCol.s[0] > 1 || tileCol.s[1] > 1 || tileCol.s[2] > 1)    //hit a light source
                    continue;
                
                //Normalize light transfer by tile area and number of rays per area of the light source
                //the constant 2.0 is an experimentally-determined factor
                lightColors[ hit->lightBaseIdx + getTileIdAt(hit, hitPos)] = 
                    add(lightColors[ hit->lightBaseIdx + getTileIdAt(hit, hitPos)],
                        mul(lightCol, (1 / (TILE_SIZE*TILE_SIZE*numSamplesPerArea*2))));
                //Color3 light = tile.getLightColor() + lightCol* (1 / (TILE_SIZE*TILE_SIZE*numSamplesPerArea*2));
                //tile.setLightColor(light);
                
                lightCol.s[0] *= 0.9 * hit->color.s[0];// = Vector3( lightCol * ;//tile.getColor();
                lightCol.s[1] *= 0.9 * hit->color.s[1];
                lightCol.s[2] *= 0.9 * hit->color.s[2];
                // prepare next (diffuse reflective) ray
                Vector3 u,v;
                createBase(hit->n, /*out*/u, /*out*/v);
                ray_dir = getCosineDistributedRandomRay(hit->n, u, v);
                pos = hitPos;
            }
            #endif
        //}
    }

    clReleaseMemObject(rectBuffer);

    char num[50];
    for ( int i = 0; i < numObjects; i++)
    {
        snprintf(num, 49, "%d", i);
        string filename = string("tiles/tile_") + num + ".png";
        saveAs( &objects[i], filename.c_str(), lightColors);
    }
    

    //Vector3 light_pos(1,1,1);
    Vector3 cam_pos = createVector3(420,882,120);
    Vector3 cam_dir = normalized( sub( createVector3(320, 205, 120), cam_pos));
    Vector3 cam_up  = createVector3(0,0,1);

    Vector3 cam_right = normalized( cross(cam_up,cam_dir) );
    cam_up   = normalized( cross(cam_right, cam_dir) );
    //std::cout << "cam_right: " << cam_right << endl;
    //std::cout << "cam_up: " << cam_up << endl;
    //std::cout << "cam_dir: " << cam_dir << endl;
    static const int img_width = 800;
    static const int img_height= 600;

    uint8_t* pixel_buffer = new uint8_t[3*img_width*img_height];
    memset(pixel_buffer, 0, 3*img_width*img_height);
    
    for (int y = 0; y < img_height; y++) 
    {
        /*if (y % 30 == 0)
        {
            cout << (y*100 / img_height) << "%" << endl;
            write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
        }*/
        for (int x = 0; x < img_width; x++) {
            /*Vector3 ray_dir = normalized( cam_dir + 
                                          cam_right* (1.25*(x-(img_width/2))/(double)img_width) + 
                                          cam_up*    (1.25*(img_height/(double)img_width)*(y-(img_height/2))/(double)img_height) );*/
            
            Vector3 rd_x = mul( cam_right,(1.25*(x-(img_width/2))/(float)img_width));
            Vector3 rd_y = mul( cam_up,   (1.25*(img_height/(float)img_width)*(y-(img_height/2))/(float)img_height));
            Vector3 ray_dir = normalized( add(cam_dir, add(rd_x, rd_y)));
            
            Vector3 col = getColor(cam_pos, ray_dir, objects, numObjects);
            /*col.r = sqrt(col.r);
            col.g = sqrt(col.g);
            col.b = sqrt(col.b);*/
            /*col.r = log(1+col.r) / log(2);  // conversion from light intensity to perceived brightness
            col.g = log(1+col.g) / log(2);
            col.b = log(1+col.b) / log(2);*/
            
            col.s[0] = 1 - exp(-col.s[0]);
            col.s[1] = 1 - exp(-col.s[1]);
            col.s[2] = 1 - exp(-col.s[2]);

          
          
            pixel_buffer[(y*img_width+x)*3 + 0] = min(col.s[0]*255, 255.0f);
            pixel_buffer[(y*img_width+x)*3 + 1] = min(col.s[1]*255, 255.0f);
            pixel_buffer[(y*img_width+x)*3 + 2] = min(col.s[2]*255, 255.0f);
        }
    }
    cout << "writing file with dimensions " << img_width << "x" << img_height << endl;
    write_png_file( "out.png", img_width, img_height, PNG_COLOR_TYPE_RGB, pixel_buffer);
    
    delete [] pixel_buffer;
    //for ( int i = 0; i < numObjects; i++)
    //    delete objects[i].tiles;
    free( objects);
    free (lightColors);

    return 0;
}
