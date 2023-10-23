#ifndef PPMM_HPP
#define PPMM_HPP

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffer.h"

#define MAX_STR_BUF 256

typedef struct RGBPixel
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGBPixel;

/**
 * @brief       .ppm image control struct
 *
 * @details     struct should be accessed using set of API functions starting with PPMImg_
 *              type is 0 for P3 (ASCII)
 *              type is 1 for P6 (binary)
 *
 */
typedef struct PPMImg
{
    int type; // 0 - ASCII 1 - bin
    size_t width;
    size_t height;
    uint8_t max_val; // max val of pixel
    struct RGBPixel *data;
} PPMImg;

// create empty
PPMImg *PPMImg_alloc();

// release memory
void PPMImg_free(PPMImg *img);

// print info
void PPMImg_print_header(PPMImg *img);

// access to pixel
RGBPixel *PPMImg_pixel_at(PPMImg *img, size_t i, size_t j);

// create from buffer of data
PPMImg *PPMImg_from_buf(const char *buf, size_t buf_size);

// write to buffer
buffer_t PPMImg_to_buf(PPMImg *img);

// create from file
PPMImg *PPMImg_from_file(const char *filename);

// write to file
void PPMImg_to_file(PPMImg *img, const char *filename);

#endif // PPMM_HPP