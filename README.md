
"globalIllumination": simulates global illumination of an apartment scene using simplified photon mapping. Its output geometry and lightmaps can be used by FlatMatch to created a globally illuminated 3D scene of the apartment to explore interactively.


Dependencies:
- `gcc` (C and C++ frontends)
- GNU `make`
- headers and libraries for `libpng`, e.g. the Ubuntu package `libpng-dev`
- OpenCL development headers, e.g. the Ubuntu package `opencl-headers`
- either
  - an OpenCL implementation ("ICD") of your _GPU_ vendor (AMD and Nvidia have been tested), or
  - an OpenCL implementation supporting your _CPU_ (AMD and Intel SDKs have been tested)
    - e.g. the Ubuntu package `beignet-dev`

Build instructions:
- to select a CPU-based implementation change both occurrences of `CL_DEVICE_TYPE_GPU` in `global_illumination_cl.c` to `CL_DEVICE_TYPE_CPU`
- make sure that only one OpenCL ICD is installed (otherwise the code around `clGetPlatformID` would need to be modified)
- "make" (for GPU-based computations)
- Note: this project mixes C and C++ code. This is not a good coding style, but happened when adding the C API-based OpenCL renderer

Compilation Output:
- executable `globalIllumination`

Usage:
* `./globalIllumination <layout image>` reads the color-coded apartment layout (see `parseLayout.c` for the color hex codes used to color-code the apartment geometry) PNG image from <layout image>, and computes apartment geometry and lightmaps
* alternatively, call `./generate_flatmatch_entry.py <layout_image> <offer_id> <scale> <latitude> <longitude> <yaw> <level>` with
  * `<layout_image>` the path to the source file in PNG format, for example the included `example.png`
  * `<offer_id>` an arbitrary numerical id
  * `<scale>` the scale of the layout image, in pixels/m
  * `<latitude>` the apartment's geographic latitude
  * `<longitude>` the apartment's geographic longitude
  * `<yaw>` the geographic direction (in degrees) in which "up" in the layout image is pointing (0 is north, 90 is east, etc.). 
  * `<level>` the building floor/level at which the apartment is located. Zero is the ground floor, a level of "x" moves the apartment floor 3.5*x meters above the ground

Output:
* the first call generates
  * set of lightmap textures in `tiles/`
  * an RLE-encoded collision map `collisionMap.json` of the same dimensions as the input image, stored as JSON. Each number in the JSON array determines *how many* successive pixels are passable/impassable. The map starts with passable pixels and then alternates between impassable and passable.
  * the 3D apartment geometry in `geometry.json`. Each entry in the "geometry" array describes a rectangle in 3D space. Texture coordinates are implicit, i.e. for each rectangle the point
    * "pos" has texture coordinates (0,0)
    * "pos + width" has texture coordinates (1, 0)
    * "pos + height" has texture coordinates (0, 1)
    * "pos + width + height" has texture coordinates (1, 1)
* the second call generates a full REST folder structure that can directly be used in a FlatMatch web application via the URL `flatview.html?rowid=<offer_id>`

