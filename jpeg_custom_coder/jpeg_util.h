#ifndef JPEG_UTIL_H
#define JPEG_UTIL_H

/**
 * @file jpeg_util.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-10
 * 
 * @copyright Copyright (c) 2023
 * 
 * File contains small utility functions for inlining
 */

#include <stdint.h>
#include <stdlib.h>

//Memory order as big endian.
//On little-endian machines: 0xhilo -> 0xlohi which looks as 0xhi 0xlo in memory
//On big-endian machines: leave 0xhilo unchanged
//Function from https://github.com/serge-rgb/TinyJPEG
/**
 * @brief Convert to BigEndian when required
 * 
 * @param native_word 
 * @return uint16_t
 */
static inline uint16_t tjei_be_word(const uint16_t native_word)
{
    uint8_t bytes[2];
    uint16_t result;
    bytes[1] = (native_word & 0x00ff);
    bytes[0] = ((native_word & 0xff00) >> 8);
    memcpy(&result, bytes, sizeof(bytes));
    return result;
}

//stolen from https://github.com/serge-rgb/TinyJPEG
static inline float fdct_1d(int u, int v, float* data)
{
#define kPI 3.14159265f
    float res = 0.0f;
    float cu = (u == 0) ? 0.70710678118654f : 1;
    float cv = (v == 0) ? 0.70710678118654f : 1;
    for ( int y = 0; y < 8; ++y ) {
        for ( int x = 0; x < 8; ++x ) {
            res += (data[y * 8 + x]) *
                    cosf(((2.0f * x + 1.0f) * u * kPI) / 16.0f) *
                    cosf(((2.0f * y + 1.0f) * v * kPI) / 16.0f);
        }
    }
    res *= 0.25f * cu * cv;
    return res;
#undef kPI
}

// Returns all code sizes from the BITS specification (JPEG C.3)
//stolen from https://github.com/serge-rgb/TinyJPEG
static inline uint8_t* tjei_huff_get_code_lengths(uint8_t huffsize[/*256*/], uint8_t const * bits)
{
    int k = 0;
    for ( int i = 0; i < 16; ++i ) {
        for ( int j = 0; j < bits[i]; ++j ) {
            huffsize[k++] = (uint8_t)(i + 1);
        }
        huffsize[k] = 0;
    }
    return huffsize;
}

// Fills out the prefixes for each code.
//stolen from https://github.com/serge-rgb/TinyJPEG
static inline uint16_t* tjei_huff_get_codes(uint16_t codes[], uint8_t* huffsize, int64_t count)
{
    uint16_t code = 0;
    int k = 0;
    uint8_t sz = huffsize[0];
    for(;;) {
        do {
            assert(k < count);
            codes[k++] = code++;
        } while (huffsize[k] == sz);
        if (huffsize[k] == 0) {
            return codes;
        }
        do {
            code = (uint16_t)(code << 1);
            ++sz;
        } while( huffsize[k] != sz );
    }
}

//stolen from https://github.com/serge-rgb/TinyJPEG
static inline void tjei_huff_get_extended(uint8_t* out_ehuffsize,
                                   uint16_t* out_ehuffcode,
                                   uint8_t const * huffval,
                                   uint8_t* huffsize,
                                   uint16_t* huffcode, int64_t count)
{
    int k = 0;
    do {
        uint8_t val = huffval[k];
        out_ehuffcode[val] = huffcode[k];
        out_ehuffsize[val] = huffsize[k];
        k++;
    } while ( k < count );
}

/**
 * @brief Get magnitude of number
 * 
 * @param value int number
 * @param out [0] - bit representation of 'value' [1] - min pow2 to represent the 'value'
 */
static inline void get_magnitude(int value, uint16_t out[2])
{
    int abs_val = value;
    if ( value < 0 ) {
        abs_val = -abs_val;
        --value;
    }
    out[1] = 1;
    while( abs_val >>= 1 ) {
        ++out[1];
    }
    
    out[0] = (uint16_t)(value & ((1 << out[1]) - 1));
}

/**
 * @brief converts pixel from RGB to YCbCr colorspace
 * 
 * @param red 
 * @param green 
 * @param blue 
 * @param res[out]
 */
static inline void rgb_to_ycbcr(uint8_t red, uint8_t green, uint8_t blue, float res[3])
{
    res[0] = 0.299 * red + 0.587 * green + 0.114 * blue - 128; // Y
    res[1] = -0.1687 * red - 0.3313 * green + 0.5 * blue;      // Cb
    res[2] = 0.5 * red - 0.4187 * green - 0.0813 * blue;       // Cr
}

/**
 * @brief Write huffman tables to file
 * 
 * @note DHT section of JPEG file interchange format
 * 
 * @note matrix_len contains count of codes for each length 1...15 inclusive
 *       matrix_val contains data (uint8_t) which is coded with huffcodes
 * 
 * @param state 
 * @param matrix_len 
 * @param matrix_val 
 * @param ht_class 
 * @param id 
 */
static inline void write_DHT(FILE* state,
                           uint8_t const * matrix_len,
                           uint8_t const * matrix_val,
                           int ht_class,
                           uint8_t id)
{

    int num_values = 0;
    for ( int i = 0; i < 16; ++i ) {
        num_values += matrix_len[i];
    }

    uint16_t DHT = tjei_be_word(0xffc4);
    // 2(len) + 1(Tc|th) + 16 (num lengths) + ?? (num values)
    uint16_t len = tjei_be_word(2 + 1 + 16 + (uint16_t)num_values);
    assert(id < 4);
    uint8_t tc_th = (uint8_t)((((uint8_t)ht_class) << 4) | id);

    fwrite(&DHT, sizeof(uint16_t), 1, state);
    fwrite(&len, sizeof(uint16_t), 1, state);
    fwrite(&tc_th, sizeof(uint8_t), 1, state);
    fwrite(matrix_len, sizeof(uint8_t), 16, state);
    fwrite(matrix_val, sizeof(uint8_t), num_values, state);
}

//JFIF headers helpers
//stolen from https://github.com/serge-rgb/TinyJPEG
#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint16_t SOI;
    // JFIF header.
    uint16_t APP0;
    uint16_t jfif_len;
    uint8_t  jfif_id[5];
    uint16_t version;
    uint8_t  units;
    uint16_t x_density;
    uint16_t y_density;
    uint8_t  x_thumb;
    uint8_t  y_thumb;
} TJEJPEGHeader;

typedef struct
{
    uint16_t com;
    uint16_t com_len;
    char     com_str[28];
} TJEJPEGComment;

// Helper struct for TJEFrameHeader (below).
typedef struct
{
    uint8_t  component_id;
    uint8_t  sampling_factors;    // most significant 4 bits: horizontal. 4 LSB: vertical (A.1.1)
    uint8_t  qt;                  // Quantization table selector.
} TJEComponentSpec;

typedef struct
{
    uint16_t         SOF;
    uint16_t         len;                   // 8 + 3 * frame.num_components
    uint8_t          precision;             // Sample precision (bits per sample).
    uint16_t         height;
    uint16_t         width;
    uint8_t          num_components;        // For this implementation, will be equal to 3.
    TJEComponentSpec component_spec[3];
} TJEFrameHeader;

typedef struct
{
    uint8_t component_id;                 // Just as with TJEComponentSpec
    uint8_t dc_ac;                        // (dc|ac)
} TJEFrameComponentSpec;

typedef struct
{
    uint16_t              SOS;
    uint16_t              len;
    uint8_t               num_components;  // 3.
    TJEFrameComponentSpec component_spec[3];
    uint8_t               first;  // 0
    uint8_t               last;  // 63
    uint8_t               ah_al;  // o
} TJEScanHeader;
#pragma pack(pop)

#endif //JPEG_UTIL_H