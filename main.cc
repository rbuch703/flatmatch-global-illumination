
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


Rectangle* objects = NULL;
cl_int numObjects;

Vector3* lightColors = NULL;
cl_int numLightColors;

/*
Vector3 getDiffuseSkyRandomRay(Vector3 ndir, Vector3 udir, Vector3 vdir)
{
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
}*/

//Builds an arbitrary orthogonal coordinate system, with one of its axes being 'ndir'
/*
void createBase(Vector3 ndir, Vector3 &c1, Vector3 &c2) {
    c1 = createVector3(0,0,1);
    if (fabs(dot(c1, ndir)) == 1) //are colinear --> cannot build coordinate base
        c1 = createVector3(0,1,0);
        
    c2 = normalized( cross(c1,ndir));
    c1 = normalized( cross(c2,ndir));
}*/

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

/*
Vector3 getObjectColorAt(Rectangle* rect, Vector3 pos)
{
    
    Vector3 light = lightColors[ rect->lightBaseIdx + getTileIdAt(rect, pos)];
    return createVector3( light.s[0] * rect->color.s[0],
                          light.s[1] * rect->color.s[1],
                          light.s[2] * rect->color.s[2]);
}*/

#if 0
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
#endif

void loadGeometry()
{
    
    vector<Rectangle> rects = parseLayout("out.png", 1000.0/720); //720px ^= 1000cm --> 1000/720 cm/px
    numObjects = rects.size();
    //are to be passed to openCL --> have to be aligned to 16byte boundary
    if (0 != posix_memalign((void**)&objects, 16, numObjects * sizeof(Rectangle))) return;

    int idx = 0;
    for ( vector<Rectangle>::const_iterator it = rects.begin(); it != rects.end(); it++)
    {
        //cout << "rectangle has " << getArea(&(*it)) / getNumTiles(&(*it)) << "cm²/tile" << endl;
    
        objects[idx++] = *it;
    }

    numLightColors = 0;
    for ( int i = 0; i < numObjects; i++)
    {
        objects[i].lightBaseIdx = numLightColors;
        objects[i].lightNumTiles = getNumTiles(&objects[i]);
        numLightColors += getNumTiles(&objects[i]);
    }

    cout << "total of " << numLightColors << " individual light texels" << endl;

    if (0 != posix_memalign( (void**) &lightColors, 16, numLightColors * sizeof(Vector3)))
    {
        assert(false);
        exit(0);
    }
    
    for (int i = 0; i < numLightColors; i++)
        lightColors[i] = createVector3(0,0,0);
}

const char* getDeviceString(cl_device_id dev_id)
{
    static char deviceString[1000];
    deviceString[0] = 0;
    
    #ifndef NDEBUG
    cl_int status =
    #endif
        clGetDeviceInfo( dev_id,   CL_DEVICE_NAME  , 1000, deviceString, NULL);
    assert(status == CL_SUCCESS);
    
  	return deviceString;
}

const char* getPlatformString(cl_platform_id platform_id)
{
    static char str[1000];
    str[0] = 0;
    
    #ifndef NDEBUG
    cl_int status =
    #endif
        clGetPlatformInfo( platform_id, CL_PLATFORM_NAME, 1000, str, NULL);
    assert(status == CL_SUCCESS);
    
  	return str;
}


void clErrorFunc ( const char *errinfo, const void * /*private_info*/, size_t /*cb*/, void * /*user_data*/)
{
    cout << errinfo << endl;
}


list<pair<cl_platform_id, cl_device_id>> getEnvironments()
{
    list<pair<cl_platform_id, cl_device_id>> environments;
    cl_uint numPlatforms = 0;	//the NO. of platforms
    clGetPlatformIDs(0, NULL, &numPlatforms);
    
	cl_platform_id *platforms = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
	clGetPlatformIDs(numPlatforms, platforms, NULL);
	for (cl_uint i = 0; i < numPlatforms; i++)
	{
        cl_uint numDevices = 0;
	    clGetDeviceIDs(platforms[i],  CL_DEVICE_TYPE_ALL , 0, NULL, &numDevices);
	    
	    cl_device_id* devices = (cl_device_id*) malloc(numDevices * sizeof(cl_device_id));
        clGetDeviceIDs(platforms[i],  CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
	    for (cl_uint j = 0; j < numDevices; j++)
	        environments.push_back( pair<cl_platform_id, cl_device_id>(platforms[i], devices[j]));

        free(devices);
	}
    free(platforms);
    return environments;
}


/* heuristic: find a platform/device - combination that is GPU-based
 *            if none exists, fall back to a CPU-based one
 */ 
void getFittingEnvironment( cl_platform_id /*out*/*platform_id, cl_device_id /*out*/*device_id)
{
    list<pair<cl_platform_id, cl_device_id>> environments = getEnvironments();

    cout << "found " << environments.size() << " CL compute environment(s):" << endl;
    for (list<pair<cl_platform_id, cl_device_id>>::const_iterator env = environments.begin(); env != environments.end(); env++)
    {
        cout << "platform '" << getPlatformString(env->first) << "', device '" << getDeviceString(env->second) << "' (" << env->second << ")" << endl;
    }
    
    // traverse the list from end to start. This heuristic helps to select a non-primary GPU as the device (if present)
    // Since the non-primary GPU is usually not concerned with screen rendering, using it with OpenCL should not impact
    // computer usage, while using the primary GPU may result in screen freezes during heavy OpenCL load
    for (list<pair<cl_platform_id, cl_device_id>>::reverse_iterator env = environments.rbegin(); env != environments.rend(); env++)
    {
        cl_device_type device_type = 0;
        	cl_int res = clGetDeviceInfo (env->second, CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL);
        	assert(res == CL_SUCCESS);
        
        if ( device_type == CL_DEVICE_TYPE_ACCELERATOR || device_type == CL_DEVICE_TYPE_GPU)
        //if ( device_type == CL_DEVICE_TYPE_CPU)
        {
            *platform_id = env->first;
            *device_id = env->second;
            return;
        }
    }
    
    //2nd pass: no GPU or dedicated accelerator found --> fall back to CPU or default implementations
    assert (environments.size() >= 1 && "No suitable OpenCL-capable device found");
    list<pair<cl_platform_id, cl_device_id>>::const_iterator env = environments.begin();
    
    *platform_id = env->first;
    *device_id = env->second;
    
    env++;
    //test code: (arbitrarily) select the second platform
    /*if (env != environments.end())
    {
        *platform_id = env->first;
        *device_id = env->second;
    }*/
    return;
    
}

void initCl(cl_context *ctx, cl_device_id *device_id) {
    
    
    /*
    cout << "CL_INVALID_DEVICE:  " << CL_INVALID_DEVICE << endl;
    cout << "CL_INVALID_VALUE:   " << CL_INVALID_VALUE << endl;
    cout << "CL_INVALID_PLATFORM:" << CL_INVALID_PLATFORM << endl;
    cout << "CL_DEVICE_NOT_AVAILABLE" << CL_DEVICE_NOT_AVAILABLE << endl;
    cout << "CL_DEVICE_NOT_FOUND" << CL_DEVICE_NOT_FOUND << endl;
    cout << "CL_OUT_OF_HOST_MEMORY" << CL_OUT_OF_HOST_MEMORY << endl;
    cout << "CL_INVALID_DEVICE_TYPE" << CL_INVALID_DEVICE_TYPE << endl;*/

    cl_platform_id platform_id;
    getFittingEnvironment( &platform_id, device_id);
    cout << "selected platform " << (platform_id) << ", device " << (*device_id) << endl;
    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id, 0};

    cl_int status;
    *ctx = clCreateContext(properties, 1/*numDevices*/, device_id, clErrorFunc,NULL,&status);
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

	//status = clBuildProgram(program, 1,&device,"-g -cl-opt-disable",NULL,NULL);
	//status = clBuildProgram(program, 1,&device,NULL,NULL,NULL);
	status = clBuildProgram(program, 1,&device,"-cl-fast-relaxed-math",NULL,NULL);
    cout << "clBuildProgram: " << status << endl;
	//assert(status == CL_SUCCESS);
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
        if (objects[i].pos.s[2] == 0.90 * 100)
        //Vector3 col = objects[i].color;
        //if (col.s[0] > 1 || col.s[1] > 1 || col.s[2] > 1)
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
    //cout << "rectBuffer: " << rectBuffer << endl;

    /* ================= */
    /* SET ACCURACY HERE */
    /* ================= */
    static const int numSamplesPerArea = 10000;

    cl_int st = 0;
    cl_int status = 0;

    cl_mem lightColorsBuffer = clCreateBuffer(ctx, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, numLightColors * sizeof(cl_float3),(void *) lightColors, &st);
    status |= st;
    
    size_t maxWorkGroupSize = 0;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkGroupSize), &maxWorkGroupSize, NULL);
    cout << "Maximum workgroup size for this device is " << maxWorkGroupSize << endl;

    for ( unsigned int i = 0; i < windows.size(); i++)
    {
        Rectangle window = windows[i];

        Vector3 xDir= getWidthVector(  &window);
        Vector3 yDir= getHeightVector( &window);
        
        //float area = xDir.length() * yDir.length();
        float area = length(xDir) * length(yDir);
        uint64_t numSamples = numSamplesPerArea * area/ 100; //  the OpenCL kernel does 100 iterations per call)
        numSamples = (numSamples / maxWorkGroupSize) * maxWorkGroupSize;    //must be a multiple of the local work size
         
	    cl_mem windowBuffer = clCreateBuffer(ctx, CL_MEM_READ_ONLY| CL_MEM_USE_HOST_PTR , sizeof(Rectangle),(void *)&window, &st );
	    status |= st;
    	if (status)
        	cout << "preparation errors: " << status << endl;

        cl_kernel kernel = clCreateKernel(prog,"photonmap", &st);
	    status |= st;

        status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&windowBuffer);
        status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&rectBuffer);
        status |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&numObjects);
        status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&lightColorsBuffer);
        //status |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&numLightColors);
        status |= clSetKernelArg(kernel, 4, sizeof(cl_float), (void *)&TILE_SIZE);
    	if (status)
        	cout << "preparation errors: " << status << endl;

        //FIXME: rewrite this code to be able to always have two kernels enqueued at the same time to maximize 
        //       performance (but not more than two kernels as this stresses GPUs too much
        
        while (numSamples)
        {
            cout << "\rPhoton-Mapping window " << (i+1) << "/" << windows.size() << " with " << (int)(numSamples*100/1000000) << "M samples   " << flush;
            
            cl_int idx = rand();
            status |= clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&idx);
            
            size_t workSize = numSamples < maxWorkGroupSize*40 ? numSamples : maxWorkGroupSize*40;  //openCL becomes unstable on ATI GPUs with work sizes > 40000 items
            numSamples -=workSize;

            	st = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &workSize, &maxWorkGroupSize, 0, NULL, NULL);
            //clFinish(queue);
            	if (st)
                	cout << "Kernel execution error: " << st << endl;
            clFinish(queue);
    	    }
    	    cout << endl;
        	
        clReleaseKernel(kernel);
        clReleaseMemObject(windowBuffer);
    }
    clFinish(queue);
    clEnqueueReadBuffer(queue, lightColorsBuffer, CL_TRUE, 0, numLightColors * sizeof(cl_float3),  (void *) lightColors, 0, NULL, NULL);

    clFinish(queue);
    clReleaseMemObject(rectBuffer);
    clReleaseMemObject(lightColorsBuffer);

    //FIXME: normalize lightColors by actual tile size (which varies from rect to rect)
    for ( int i = 0; i < numLightColors; i++)
        lightColors[i] = div_vec3(lightColors[i], (TILE_SIZE*TILE_SIZE * 4 * numSamplesPerArea));

    for ( unsigned int i = 0; i < windows.size(); i++)
    {
        Rectangle window = windows[i];
        for (int j = 0; j < getNumTiles(&window); j++)
            lightColors[ window.lightBaseIdx + j] = createVector3(1, 1, 1);
    }


    char num[50];
    for ( int i = 0; i < numObjects; i++)
    {
        snprintf(num, 49, "%d", i);
        string filename = string("tiles/tile_") + num + ".png";
        saveAs( &objects[i], filename.c_str(), lightColors);
    }
    

    //Vector3 light_pos(1,1,1);
    /*Vector3 cam_pos = createVector3(420,882,120);
    Vector3 cam_dir = normalized( sub( createVector3(320, 205, 120), cam_pos));
    Vector3 cam_up  = createVector3(0,0,1);

    Vector3 cam_right = normalized( cross(cam_up,cam_dir) );
    cam_up   = normalized( cross(cam_right, cam_dir) );*/
    //std::cout << "cam_right: " << cam_right << endl;
    //std::cout << "cam_up: " << cam_up << endl;
    //std::cout << "cam_dir: " << cam_dir << endl;
    /*
    static const int img_width = 800;
    static const int img_height= 600;

    uint8_t* pixel_buffer = new uint8_t[3*img_width*img_height];
    memset(pixel_buffer, 0, 3*img_width*img_height);

    
    for (int y = 0; y < img_height; y++) 
    {
        for (int x = 0; x < img_width; x++) {
            
            Vector3 rd_x = mul( cam_right,(1.25*(x-(img_width/2))/(float)img_width));
            Vector3 rd_y = mul( cam_up,   (1.25*(img_height/(float)img_width)*(y-(img_height/2))/(float)img_height));
            Vector3 ray_dir = normalized( add(cam_dir, add(rd_x, rd_y)));
            
            Vector3 col = getColor(cam_pos, ray_dir, objects, numObjects);
            
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
    */
    //for ( int i = 0; i < numObjects; i++)
    //    delete objects[i].tiles;
    free( objects);
    free (lightColors);

    return 0;
}
