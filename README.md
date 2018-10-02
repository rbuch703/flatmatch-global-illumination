
"globalIllumination": simulates global illumination of an apartment scene using simplified photon mapping. Its output geometry and lightmaps can be used by FlatMatch to created a globally illuminated 3D scene of the apartment to explore interactively.


Dependencies:
- gcc (C and C++ frontends)
- GNU make
- headers and libraries for libpng, e.g. the Ubuntu package "libpng-dev"
- OpenCL development headers, e.g. the Ubuntu package "opencl-headers"
- either
  - an OpenCL implementation ("ICD") of your _GPU_ vendor (AMD and Nvidia have been tested), or
  - an OpenCL implementation supporting your _CPU_ (AMD and Intel SDKs have been tested)
    - e.g. the Ubuntu package "beignet-dev"

Build instructions:
- to select a CPU-based implementation change both occurrences of CL_DEVICE_TYPE_GPU in global_illumination_cl.c to CL_DEVICE_TYPE_CPU
- make sure that only one OpenCL ICD is installed (otherwise the code around "clGetPlatformID" would need to be modified)
- "make" (for GPU-based computations)
- Note: this project mixes C and C++ code. This is not a good coding style, but happened when adding the C API-based OpenCL renderer

Compilation Output:
- executable "globalIllumination"

Usage:
- "./globalIllumination <layout image>" reads the color-coded apartment layout (see parseLayout.c for the color hex codes used to color-code the apartment geometry) PNG image from <layout image>, and computes apartment geometry and lightmaps

Output:
- set of lightmap textures in "tiles/"
- an RLE-encoded collision map "collisionMap.json" of the same dimensions as the input image, stored as JSON. Each number in the JSON array determines *how many* successive pixels are passable/impassable. The map starts with passable pixels and then alternates between impassable and passable.
- the 3D apartment geometry in "geometry.json"
  - each entry in the "geometry" array describes a rectangle in 3D space. Texture coordinates are implicit, i.e. for each rectangle the point
    - "pos" has texture coordinates (0,0)
    - "pos + width" has texture coordinates (1, 0)
    - "pos + height" has texture coordinates (0, 1)
    - "pos + width + height" has texture coordinates (1, 1)

