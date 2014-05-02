
"radiosity": (misnomer for historical reasons) simulates global illumination of an apartment scene using simplified photon mapping

Dependencies:
- gcc (C and C++ frontends)
- GNU make
- headers and libraries for libpng, e.g. the Ubuntu package "libpng12-dev"
- OpenCL development headers and either (1) an OpenCL implementation ("ICD") of your GPU vendor (AMD and Nvidia have been tested), or (2) an OpenCL implementation supporting your CPU (AMD and Intel SDKs have been tested)

Build instructions:
- to select a CPU-based implementation change both occurrences of CL_DEVICE_TYPE_GPU in main.cc to CL_DEVICE_TYPE_CPU
- make sure that only one OpenCL ICD is installed (otherwise the code around "clGetPlatformID" would need to be modified)
- "make" (for GPU-based computations)
- Note: this project mixes C and C++ code. This is not a good coding style, but happened when adding the C API-based OpenCL renderer

Compilation Output:
- "radiosity"

Usage:
- "./radiosity" reads the apartment layout from "out.png" (hard-coded), computes lightmaps, and saves those as textures in the folder "tiles"

Output:
- The resulting lightmaps can be used by "flatgl" to created a globally illuminated 3D scene of the apartment to explore interactively
