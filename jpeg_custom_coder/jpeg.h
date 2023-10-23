#ifndef JPEG_H
#define JPEG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ppmm.h"
#include "jpeg_util.h"
#include "jpeg_tables.h"
#include "buffer.h"
#include "dct.h"

/**
 * @brief   Baseline DCT JPEG Encoder of RGB data
 * 
 * @details 'compression_lvl' affects which quantization tables are used
 *          'compression_lvl' defaults to '1'
 *          1 - no compression (tables filled with '1')
 *          2 - basic compression (tables from irfan viewer)
 *          3 - high compression (JPEG spec defaults)
 * 
 *          Encoder doesn't generate tables on the go
 *          So predefined tables are used
 *          each table corresponds to specific coeff. being encoded
 *          0 Luma DC; 1 Luma AC; 2 Chroma DC; 3 Chroma AC
 *          'ehuffsize' - extended sizes in bits
 *          'ehuffcode' - extended codes
 *          'ht_bits' - default sizes
 *          'ht_vals' - default values
 * 
 *          'width' - width in pixels
 *          'height' - height in pixels
 * 
 *          'q_table' - quantization tables
 *          1 - Luma; 2 - Chroma
 * 
 *          'result' - after encoding contains encoded data 
 *          (Start of Scan/SOS segment JPEG spec.) 
 */
struct jpeg_encoder {
    int use_fdct;
    int compression_lvl;

    uint8_t         ehuffsize[4][257];
    uint16_t        ehuffcode[4][256];
    uint8_t const * ht_bits[4];
    uint8_t const * ht_vals[4];

    uint16_t width;
    uint16_t height;
    uint8_t q_table[2][64];//Quantization table
    float* fdct_q_table[2];

    buffer_t result;
};

//convenience typedef
typedef struct jpeg_encoder* jpeg_encoder_t;

/**
 * @brief   allocate jpeg encoder object
 * 
 * @details allocates jpeg encoder object and sets up basic settings
 *          dont forget to call jpeg_free()
 * 
 * @return jpeg_encoder_t 
 */
jpeg_encoder_t jpeg_alloc();

/**
 * @brief free allocated data for encoder
 * 
 */
void jpeg_free(jpeg_encoder_t);

/**
 * @brief Creates default Huffman tables
 * 
 * @param enc jpeg encoder instance
 */
void jpeg_setup_default_huffman_tables(jpeg_encoder_t enc);

void _jpeg_copy_table(uint8_t* dest, const uint8_t* src);

/**
 * @brief Creates default quantization tables
 * 
 * @param enc 
 */
void jpeg_setup_q_tables(jpeg_encoder_t enc);

/**
 * @brief Encodes data into enc->result buffer
 * 
 * @param enc 
 * @param width 
 * @param height 
 * @param data 
 */
void jpeg_encode_data(jpeg_encoder_t enc, unsigned width, unsigned height, const uint8_t* data);

/**
 * @brief Writes data from enc->result into file
 * 
 * @param enc 
 * @param filename 
 */
void jpeg_write_to_file(jpeg_encoder_t enc, const char* filename);

/**
 * @brief Bitstack
 * 
 * @note Function adapted from https://github.com/serge-rgb/TinyJPEG
 * 
 * @param enc 
 * @param bitbuffer 
 * @param location 
 * @param num_bits 
 * @param bits 
 */
static inline void tjei_write_bits(jpeg_encoder_t enc,
                                       uint32_t* bitbuffer, uint32_t* location,
                                       uint16_t num_bits, uint16_t bits)
{
    //   v-- location
    //  [                     ]   <-- bit buffer
    // 32                     0
    //
    // This call pushes to the bitbuffer and saves the location. Data is pushed
    // from most significant to less significant.
    // When we can write a full byte, we write a byte and shift.

    // Push the stack.
    uint32_t nloc = *location + num_bits;
    *bitbuffer |= (uint32_t)(bits << (32 - nloc));
    *location = nloc;
    while ( *location >= 8 ) {
        // Grab the most significant byte.
        uint8_t c = (uint8_t)((*bitbuffer) >> 24);
        // Write it to file.
        buffer_append(enc->result, &c, 1);
        if ( c == 0xff )  {
            // Special case: tell JPEG this is not a marker.
            uint8_t z = 0;
            buffer_append(enc->result, &z, 1);
        }
        // Pop the stack.
        *bitbuffer <<= 8;
        *location -= 8;
    }
}

typedef struct {
    uint8_t category: 4;
    uint8_t zeroes: 4;
} byte_nibble;

#ifdef __cplusplus
}
#endif

#endif // JPEG_H