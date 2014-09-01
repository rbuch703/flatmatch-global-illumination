
#include "rectangle.h"
#include "parseLayout.h"
//#include "png_helper.h"
#include "vector3_cl.h"

#include <list>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <stdlib.h> //for rand()
//#include <string>
#include <string.h> //for memset
using namespace std;


Rectangle* walls = NULL;
cl_int numWalls;

vector<Rectangle> windows, lights, box;

Vector3* lightColors = NULL;
cl_int numTexels;
int width, height;

pair<float, float> startingPos;


void loadGeometry(string filename, float scale)
{
    vector<Rectangle> vWalls;
    parseLayout(filename.c_str(), scale, width, height, vWalls, windows, lights, box, startingPos);
    
    //the set of walls will be uploaded to OpenCL, and thus needs to be converted to a suitable structure (array of structs)
    //the windows are only uploaded only one by one, so no conversion is necessary here
    numWalls = vWalls.size();
    //are to be passed to OpenCL --> have to be aligned to 16byte boundary
    if (0 != posix_memalign((void**)&walls, 16, numWalls * sizeof(Rectangle))) 
    {
        cout << "[Err] aligned memory allocation failed, exiting ..." << endl;
        exit(0);
    }

    for (unsigned int i = 0; i < vWalls.size(); i++)
        walls[i] = vWalls[i];

    numTexels = 0;
    for ( int i = 0; i < numWalls; i++)
    {
        walls[i].lightmapSetup.s[0] = numTexels;
        //objects[i].lightNumTiles = getNumTiles(&objects[i]);
        numTexels += getNumTiles(&walls[i]);
    }

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

    //cout << "found " << environments.size() << " CL compute environment(s):" << endl;
    /*
    for (list<pair<cl_platform_id, cl_device_id>>::const_iterator env = environments.begin(); env != environments.end(); env++)
    {
        cout << "platform '" << getPlatformString(env->first) << "', device '" << getDeviceString(env->second) << "' (" << env->second << ")" << endl;
    }*/
    
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
    
    cl_platform_id platform_id;
    getFittingEnvironment( &platform_id, device_id);
    cout << "[INF] Selected device '" << getDeviceString(*device_id) << "'" << endl << endl;//"' on platform '" << getPlatformString(platform_id) << "' , " << endl << endl;
    
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
    //cout << "clCreateProgramWithSource: " << status << endl;
	assert(status == CL_SUCCESS);

	//status = clBuildProgram(program, 1,&device,"-g -cl-opt-disable",NULL,NULL);
	//status = clBuildProgram(program, 1,&device,NULL,NULL,NULL);
	status = clBuildProgram(program, 1,&device,"-cl-fast-relaxed-math",NULL,NULL);
    //cout << "clBuildProgram: " << status << endl;

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
    }
    return program;
}


void photonMapLightSource(cl_context ctx, cl_command_queue queue, cl_kernel kernel, const Rectangle &lightSource, float numSamplesPerArea, cl_int isWindow, size_t maxWorkGroupSize,
                          cl_mem rectBuffer, cl_mem lightColorsBuffer)
{
    Vector3 xDir= getWidthVector(  &lightSource);
    Vector3 yDir= getHeightVector( &lightSource);
    
    float area = length(xDir) * length(yDir);
    uint64_t numSamples = (numSamplesPerArea * area) / 100; //  the OpenCL kernel does 100 iterations per call)
    numSamples = (numSamples / maxWorkGroupSize + 1) * maxWorkGroupSize;    //must be a multiple of the local work size
    cl_int st = 0;

    cl_mem windowBuffer = clCreateBuffer(ctx, CL_MEM_READ_ONLY| CL_MEM_USE_HOST_PTR , sizeof(Rectangle),(void *)&lightSource, &st );
	if (st)
    	cout << "buffer creation preparation errors: " << st << endl;

    st |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&windowBuffer);
    st |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&rectBuffer);
    st |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&numWalls);
    st |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&lightColorsBuffer);
    st |= clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&isWindow);
	if (st)
	{
    	cout << "Kernel Arg preparation errors: " << st << endl;
    	exit(0);
	}

    //FIXME: rewrite this code to be able to always have two kernels enqueued at the same time to maximize 
    //       performance (but not more than two kernels as this stresses GPUs too much
    while (numSamples)
    {
        cout << "\rPhoton-Mapping window " << " with " << (int)(numSamples*100/1000000) << "M samples   " << flush;
        
        cl_int idx = rand();
        st |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&idx);
        if (st)
            exit(-1);
        size_t workSize = numSamples < maxWorkGroupSize * 40 ? numSamples : maxWorkGroupSize * 40;  //openCL becomes unstable on ATI GPUs with work sizes > 40000 items
        numSamples -=workSize;

    	st |= clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &workSize, &maxWorkGroupSize, 0, NULL, NULL);
        //clFinish(queue);
    	if (st)
    	{
        	cout << "Kernel execution error: " << st << endl;
    	    exit(-1);
    	}
    	
        clFinish(queue);
	    }
	    cout << endl;
    	
    clReleaseMemObject(windowBuffer);

}

void performGlobalIllumination(cl_context ctx, cl_device_id device, const vector<Rectangle> &windows, int numSamplesPerArea = 1000)
{
	cl_command_queue queue = clCreateCommandQueue(ctx, device, 0, NULL);
	
//    cout << "context: " << ctx << ", queue: " << queue << endl;
    char* program_str = getProgramString("photonmap.cl");

    cl_program prog = createProgram(ctx, device, program_str);
    free(program_str);


    cl_mem rectBuffer = clCreateBuffer(ctx, CL_MEM_READ_ONLY |CL_MEM_COPY_HOST_PTR, numWalls * sizeof(Rectangle),(void *) walls, NULL);

    cl_int st = 0;
    cl_int status = 0;

    cl_mem lightColorsBuffer = clCreateBuffer(ctx, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, numTexels * sizeof(cl_float3),(void *) lightColors, &st);
    status |= st;
    
    size_t maxWorkGroupSize = 0;
    cl_kernel kernel = clCreateKernel(prog,"photonmap", &st);
    clGetKernelWorkGroupInfo(kernel, device,  CL_KERNEL_WORK_GROUP_SIZE , sizeof(maxWorkGroupSize), &maxWorkGroupSize, NULL);
    //clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkGroupSize), &maxWorkGroupSize, NULL);
    //cout << "Maximum workgroup size for this kernel on this device is " << maxWorkGroupSize << endl;

    for ( unsigned int i = 0; i < windows.size(); i++)
        photonMapLightSource(ctx, queue, kernel, windows[i], numSamplesPerArea, true, maxWorkGroupSize, rectBuffer, lightColorsBuffer);

    for ( unsigned int i = 0; i < lights.size(); i++)
        photonMapLightSource(ctx, queue, kernel, lights[i], numSamplesPerArea, false, maxWorkGroupSize, rectBuffer, lightColorsBuffer);

    
    clFinish(queue);
    clEnqueueReadBuffer(queue, lightColorsBuffer, CL_TRUE, 0, numTexels * sizeof(cl_float3),  (void *) lightColors, 0, NULL, NULL);

    clFinish(queue);
    clReleaseKernel(kernel);
    clReleaseMemObject(rectBuffer);
    clReleaseMemObject(lightColorsBuffer);

}


ostream& operator<<(ostream &os, const Vector3 &vec)
{
    os <<  "[" << vec.s[0] << ", " << vec.s[1] << ", " << vec.s[2] << "]";
    return os;
}

void writeJsonOutput()
{
    ofstream jsonGeometry("geometry.json");

    jsonGeometry << "{" << endl;
    jsonGeometry << "\"startingPosition\" : [" << startingPos.first << ", " << startingPos.second << "]," <<  endl;
    jsonGeometry << "\"layoutImageSize\" : [" << width << ", " << height << "]," <<  endl;

    jsonGeometry << "\"geometry\" : [" << endl;
    char num[50];
    for ( int i = 0; i < numWalls; i++)
    {
        snprintf(num, 49, "%d", i);
        string filename = string("tiles/tile_") + num + ".png";
        saveAs( &walls[i], filename.c_str(), lightColors);
        
            //{ pos: [1,2,3], width: [4,5,6], height: [7,8,9], texture_id: 10, isWindow: false};

        jsonGeometry << "  { \"pos\": " << walls[i].pos <<
                        ", \"width\": " << walls[i].width << 
                        ", \"height\": "<< walls[i].height <<
                        ", \"textureId\": " << i << "}";
        if (i+1 < numWalls)
            jsonGeometry << ", ";
        jsonGeometry << endl;
        
    }

    jsonGeometry << "]," << endl;
    jsonGeometry << "\"box\": [" << endl;

    for (unsigned int i = 0; i < box.size(); i++)
    {
        jsonGeometry << "  { \"pos\": " << box[i].pos <<
                        ", \"width\": " << box[i].width << 
                        ", \"height\": "<< box[i].height << "}";
        if (i + 1 < box.size())
            jsonGeometry << ", ";
        jsonGeometry << endl;
    }


    jsonGeometry << "]" << endl;
    jsonGeometry << "}" << endl;
}

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
    
    //string filename = (argc >= 2) ? argv[1] : "out.png" ;
    

    float scale = argc < 3 ? 30 : atof(argv[2]);
    
    //scale is passed in the more human-readable pixel/m, but the geometry loader needs it in m/pixel
    loadGeometry(argv[1], 1/scale);

    /*vector<Rectangle> windows;
    for ( int i = 0; i < numWalls; i++)
    {
        if (objects[i].pos.s[2] == 0.90 * 100)  //HACK: select windows, which start at 90cm height
            windows.push_back( objects[i]);
    }*/
    cout << "[INF] Layout consists of " << numWalls << " walls (" << numTexels/1000000.0 << "M texels) and " << windows.size() << " windows" << endl;
    //cout << "total of " << numLightColors << " individual light texels" << endl;


    cl_context ctx;
    cl_device_id device;
	initCl(&ctx, &device);

    int numSamplesPerArea = 1000000 * 10;   // rays per square meter of window/light surface
    performGlobalIllumination(ctx, device, windows, numSamplesPerArea);
    
    for ( int i = 0; i < numWalls; i++)
    {
        Rectangle &obj = walls[i];
        float tilesPerSample = getNumTiles(&obj) / (getArea(&obj) * numSamplesPerArea);  
        int baseIdx = obj.lightmapSetup.s[0];

        for (int j = 0; j < getNumTiles(&obj); j++)
            lightColors[baseIdx + j] = mul(lightColors[baseIdx +j], 0.3 * tilesPerSample);
    }    

    //write texture files
    char num[50];
    for ( int i = 0; i < numWalls; i++)
    {
        snprintf(num, 49, "%d", i);
        string filename = string("tiles/tile_") + num + ".png";
        saveAs( &walls[i], filename.c_str(), lightColors);
    }


    writeJsonOutput();
   
    free( walls);
    free( lightColors);


    return 0;
}
