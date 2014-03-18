#ifndef PNG_HELPER_H
#define PNG_HELPER_H

void read_png_file(const char* file_name, int &width, int& height, int& color_type, uint8_t* &pixel_buffer );
void write_png_file(const char* file_name, int width, int height, int color_type, uint8_t *pixel_buffer);
static const int PNG_COLOR_TYPE_RGB  = 2;
static const int PNG_COLOR_TYPE_RGBA = 6;

#endif
