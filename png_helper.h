#ifndef PNG_HELPER_H
#define PNG_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* read/write from/to memory is disabled. The current code relies on functions of libpng-12 that are
 * no longer present in the recent libpng-16.
 */
void read_png_file(const char* file_name, int *width, int* height, int* color_type, uint8_t** pixel_buffer );
//int read_png_from_memory(const uint8_t *data, int numBytesIn, int *width, int *height, int *color_type, uint8_t** pixel_buffer );
void write_png_file(const char* file_name, int width, int height, int color_type, uint8_t *pixel_buffer);
//void write_png_to_memory(uint8_t **outData, int *outSize, int width, int height, int color_type, uint8_t *pixel_buffer);

static const int PNG_COLOR_TYPE_RGB  = 2;
static const int PNG_COLOR_TYPE_RGBA = 6;


#ifdef __cplusplus
}
#endif

#endif
