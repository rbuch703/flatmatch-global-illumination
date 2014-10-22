//#include <unistd.h>
//#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <stdarg.h>

#include <assert.h>
#include <stdint.h>
//#include <iostream>

//using namespace std;
//#define PNG_DEBUG 3
#include <png.h>

typedef struct PngIo {
    uint8_t *data;
    uint32_t dataSize;
    uint32_t position;
} PngIo;

static void readFromMemory(png_structp png_ptr, png_bytep outBytes,
   png_size_t byteCountToRead)
{
    PngIo *io = png_ptr->io_ptr;
    assert( io->position + byteCountToRead < io->dataSize);
    memcpy( outBytes, io->data + io->position, byteCountToRead);
    io->position += byteCountToRead;
}

static int read_png_core(png_structp png_ptr, png_infop info_ptr, int *width, int *height, int *color_type, uint8_t** pixel_buffer)
{
    png_read_info(png_ptr, info_ptr);

    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);
    *color_type = png_get_color_type(png_ptr, info_ptr);
    #ifndef NDEBUG
        int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    #endif
    assert(bit_depth == 8);
    unsigned int num_channels   = png_get_channels(png_ptr, info_ptr);        
    assert(( *color_type == PNG_COLOR_TYPE_RGB && num_channels ==3) || (*color_type == PNG_COLOR_TYPE_RGBA && num_channels == 4));
    //number_of_passes = 
    //png_set_interlace_handling(png_ptr); //TODO: do we need this?
    png_read_update_info(png_ptr, info_ptr);


    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
        return 0;


    assert (png_get_rowbytes(png_ptr,info_ptr) == num_channels * (*width) );
    
    *pixel_buffer = (uint8_t*)malloc( (*width)*(*height)*num_channels *  sizeof(uint8_t));
    png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++)
            row_pointers[y] = & ((*pixel_buffer)[ (*width)*y*num_channels]);

    png_read_image(png_ptr, row_pointers);

    png_destroy_read_struct(&png_ptr, &info_ptr,NULL);
    free (row_pointers);
    //fclose(fp);
    return 1;

}

int read_png_from_memory(uint8_t *data, int numBytesIn, int *width, int *height, int *color_type, uint8_t** pixel_buffer )
{
    png_structp png_ptr;
    png_infop info_ptr;
    PngIo io = { .data = data, .dataSize = numBytesIn, .position = 0};
    //unsigned char header[8];    // 8 is the maximum size that can be checked

    if (png_sig_cmp(data, 0, 8))
        return 0;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        return 0;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        return 0;
        
    if (setjmp(png_jmpbuf(png_ptr)))
        return 0;

    //png_init_io(png_ptr, fp);
    png_set_read_fn(png_ptr, &io, readFromMemory);
    //png_set_sig_bytes(png_ptr, 8);
    return read_png_core(png_ptr, info_ptr, width, height, color_type, pixel_buffer);
}

void read_png_file(const char* file_name, int *width, int *height, int *color_type, uint8_t** pixel_buffer )
{
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned char header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp) 
    {
        printf("File '%s' could not be opened, exiting ...\n", file_name);
        exit(0);
    }

    if (8 !=fread(header, 1, 8, fp))
    {
        printf("Error reading file header, exiting ...\n");
        return;
    }
        
    if (png_sig_cmp(header, 0, 8))
        return;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        return;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        return;
        
    if (setjmp(png_jmpbuf(png_ptr)))
        return;

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    read_png_core(png_ptr, info_ptr, width, height, color_type, pixel_buffer);
    fclose(fp);
}

/*
 png_set_write_fn(png_structp write_ptr,
        voidp write_io_ptr, png_rw_ptr write_data_fn,
        png_flush_ptr output_flush_fn);*/

static void writeToMemory(png_structp png_ptr, png_bytep src_data, png_size_t numBytes)
{
    //printf("supposed to write %ld bytes coming from %p\n", numBytes, src_data);
    
    PngIo *io = png_ptr->io_ptr;
    while (io->position + numBytes > io->dataSize)
    {
        //printf("resizing output array to %d bytes\n", 2*io->dataSize);
        io->data = realloc(io->data, 2*io->dataSize);
        io->dataSize *= 2;
    }
    memcpy( io->data + io->position, src_data, numBytes);
    io->position += numBytes;
    
}

static void flush( png_structp png_ptr __attribute__ ((unused)) )
{
    //printf("supposed to flush\n");
}
      
static void write_png_core(png_structp png_ptr, png_infop info_ptr, int width, int height, int color_type, uint8_t *pixel_buffer)
{
    png_write_info(png_ptr, info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
            return;

    int num_channels = color_type == PNG_COLOR_TYPE_RGB ? 3 : color_type == PNG_COLOR_TYPE_RGBA ? 4 : -1;
    assert( num_channels != -1);
    
    uint8_t *row_pointers[height];
    for (int y = 0; y < height; y++)
        row_pointers[y] = &pixel_buffer[y*width*num_channels];

    png_write_image(png_ptr, row_pointers);


    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
            return;

    png_write_end(png_ptr, NULL);

    /* cleanup heap allocation */

    //fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);

}
        
void write_png_to_memory(uint8_t **outData, int *outSize, int width, int height, int color_type, uint8_t *pixel_buffer)
{
    png_structp png_ptr;
    png_infop info_ptr;

    PngIo io = { .data = malloc(8), .dataSize = 8, .position = 0};

    /* create file */
    //FILE *fp = fopen(file_name, "wb");
    //if (!fp) return;

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        return;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        return;

    if (setjmp(png_jmpbuf(png_ptr)))
        return;
    
    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
        return;
    
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_set_write_fn(png_ptr, &io, writeToMemory, flush);

    write_png_core(png_ptr, info_ptr, width, height, color_type, pixel_buffer);

    *outData = realloc(io.data, io.position); //reduce allocated area to actually used size
    *outSize = io.position;
}
        
void write_png_file(const char* file_name, int width, int height, int color_type, uint8_t *pixel_buffer)
{
        
    png_structp png_ptr;
    png_infop info_ptr;
    /* create file */
    FILE *fp = fopen(file_name, "wb");
    if (!fp) return;

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        return;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        return;

    if (setjmp(png_jmpbuf(png_ptr)))
        return;
    

    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
        return;
    
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_init_io(png_ptr, fp);

    write_png_core(png_ptr, info_ptr, width, height, color_type, pixel_buffer);
    fclose(fp);
}

/*
int main(int argc, char **argv)
{
    int width, height, color_type;
    uint8_t *pixel_buffer;

    read_png_file("test_int.png", width, height, color_type, pixel_buffer);

    write_png_file("out.png", width, height, color_type, pixel_buffer);

        
    delete [] pixel_buffer;

    return 0;
}*/

