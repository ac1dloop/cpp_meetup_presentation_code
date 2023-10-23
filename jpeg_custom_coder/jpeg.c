#include "jpeg.h"

jpeg_encoder_t jpeg_alloc()
{
    jpeg_encoder_t enc = (jpeg_encoder_t)malloc(sizeof(struct jpeg_encoder));

    for (int k = 0; k < 4; k++){
        memset(enc->ehuffsize[k], 0, 257);
        memset(enc->ehuffcode[k], 0, 256);
    }

    enc->compression_lvl = 1;
    enc->use_fdct = 0;

    enc->result = NULL;

    return enc;
}

void jpeg_free(jpeg_encoder_t enc)
{
    assert(enc);

    buffer_free(enc->result);

    if (enc->use_fdct == 1){
        free(enc->fdct_q_table[0]);
        free(enc->fdct_q_table[1]);
    }

    free(enc);
}

void jpeg_setup_default_huffman_tables(jpeg_encoder_t enc)
{
    // Default JPEG Huffman table codes and sizes
    enc->ht_bits[0] = tjei_default_ht_luma_dc_len;
    enc->ht_bits[1] = tjei_default_ht_luma_ac_len;
    enc->ht_bits[2] = tjei_default_ht_chroma_dc_len;
    enc->ht_bits[3] = tjei_default_ht_chroma_ac_len;

    enc->ht_vals[0] = tjei_default_ht_luma_dc;
    enc->ht_vals[1] = tjei_default_ht_luma_ac;
    enc->ht_vals[2] = tjei_default_ht_chroma_dc;
    enc->ht_vals[3] = tjei_default_ht_chroma_ac;

    // How many codes in total for each of LUMA_(DC|AC) and CHROMA_(DC|AC)
    int32_t spec_tables_len[4] = {0};

    for (int i = 0; i < 4; ++i)
    {
        for (int k = 0; k < 16; ++k)
        {
            spec_tables_len[i] += enc->ht_bits[i][k];
        }
    }

    // Fill out the extended tables..
    uint8_t huffsize[4][257];
    uint16_t huffcode[4][256];
    for (int i = 0; i < 4; ++i)
    {
        assert(256 >= spec_tables_len[i]);
        tjei_huff_get_code_lengths(huffsize[i], enc->ht_bits[i]);
        tjei_huff_get_codes(huffcode[i], huffsize[i], spec_tables_len[i]);
    }
    for (int i = 0; i < 4; ++i)
    {
        int64_t count = spec_tables_len[i];
        tjei_huff_get_extended(enc->ehuffsize[i],
                               enc->ehuffcode[i],
                               enc->ht_vals[i],
                               &huffsize[i][0],
                               &huffcode[i][0], count);
    }
}

void _jpeg_copy_table(uint8_t* dest, const uint8_t* src)
{
    // for (int i = 0; i < 64; i++)
    //     dest[i] = src[zz_index[i]];
    memcpy(dest, src, 64);
}

void jpeg_setup_q_tables(jpeg_encoder_t enc)
{
    if (enc->compression_lvl == 3)
    {
        _jpeg_copy_table(enc->q_table[0], q_table_luma);
        _jpeg_copy_table(enc->q_table[1], q_table_chroma);
    }
    else if (enc->compression_lvl == 2)
    {
        _jpeg_copy_table(enc->q_table[0], irfan_q_table_luma);
        _jpeg_copy_table(enc->q_table[1], irfan_q_table_chroma);
    }
    else
    {
        _jpeg_copy_table(enc->q_table[0], test_quantize_table);
        _jpeg_copy_table(enc->q_table[1], test_quantize_table);
    }

    if (enc->use_fdct == 1){
        enc->fdct_q_table[0] = malloc(64 * sizeof(float));
        enc->fdct_q_table[1] = malloc(64 * sizeof(float));

        for(int y=0; y<8; y++) {
            for(int x=0; x<8; x++) {
                int i = y*8 + x;
                enc->fdct_q_table[0][y*8+x] = 1.0f / (8 * aan_scales[x] * aan_scales[y] * enc->q_table[0][zz_index[i]]);
                enc->fdct_q_table[1][y*8+x] = 1.0f / (8 * aan_scales[x] * aan_scales[y] * enc->q_table[1][zz_index[i]]);
            }
        }
    }
}

void jpeg_encode_data(jpeg_encoder_t enc, unsigned width, unsigned height, const uint8_t *data)
{
    assert(enc);
    assert(data);
    assert(width > 0);
    assert(height > 0);

    // width and height affects JPEG headers when writing to file
    enc->width = width;
    enc->height = height;

    jpeg_setup_q_tables(enc);

    jpeg_setup_default_huffman_tables(enc);

    // MCU (JPEG spec.) is 8x8 block of single component
    float mcu[3][64];          // after conversion to YCbCr
    float mcu_dct[64];         // after cosine transform
    int16_t mcu_zz[64];        // after zig-zag transform
    int16_t DC[3] = {0, 0, 0}; // DC coeff. for each component

    // bitstack and bit offset
    uint32_t bitstack = 0;
    uint32_t location = 0;

    // when bitstack accumulates 8 bits. byte is appended to result
    enc->result = buffer_alloc(0);

    uint8_t ac_byte;            // zero count + AC coeff.
    uint16_t mag[2];            // pow2 of value and bit representation of value
    uint8_t dc_index, ac_index; // specifies huffman and Q tables' indexes

    uint16_t W, H; // iterate image width, height
    int i, j, k;   // iterate anything
    float ycbcr[3];                       // for conversion RGB -> YCbCr
    int block_index, src_index, col, row; // iterate over MCU inside picture

    // tmp vals
    float tmp_f;
    int tmp_i, zero_i, zero_count, diff;
    // float* memory = malloc(enc->width * enc->height * sizeof(float) * 3);
    // size_t mem_offset = 0;
    for (H = 0; H < enc->height; H += 8)
    {
        for (W = 0; W < enc->width; W += 8)
        {

            for (i = 0; i < 8; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    block_index = (i * 8 + j);

                    src_index = (((H + i) * enc->width) + (W + j)) * 3;

                    col = W + j;
                    row = H + i;

                    if (row >= enc->height)
                    {
                        src_index -= (enc->width * (row - enc->height + 1)) * 3;
                    }
                    if (col >= enc->width)
                    {
                        src_index -= (col - enc->width + 1) * 3;
                    }
                    assert(src_index < enc->width * enc->height * 3);

                    rgb_to_ycbcr(
                        data[src_index + 0],
                        data[src_index + 1],
                        data[src_index + 2],
                        ycbcr);

                    for (k = 0; k < 3; k++)
                    {
                        mcu[k][block_index] = ycbcr[k];
                    }
                }
            }

            // printf("BLOCK: %d\n", block++);
            for (k = 0; k < 3; k++)
            {
                // DCT
                if (!enc->use_fdct)
                    dct_2d(mcu[k], mcu_dct);
                else 
                    tjei_fdct(mcu[k]);

                // Quantize + Zig-Zag
                for (i = 0; i < 64; i++)
                {
                    if (!enc->use_fdct){
                        if (k == 0)
                            tmp_f = mcu_dct[i] / (enc->q_table[0][i]);
                        else
                            tmp_f = mcu_dct[i] / (enc->q_table[1][i]);

                        tmp_i = (int)((tmp_f > 0) ? floorf(tmp_f + 0.5f) : ceilf(tmp_f - 0.5f));
                    } else {
                        if (k == 0)
                            tmp_f = mcu[k][i] * enc->fdct_q_table[0][i];
                        else
                            tmp_f = mcu[k][i] * enc->fdct_q_table[1][i];

                        tmp_f = floorf(tmp_f + 1024 + 0.5f);
                        tmp_f -= 1024;
                        tmp_i = (int)(tmp_f);
                    }


                    mcu_zz[zz_index[i]] = tmp_i;
                }

                if (k == 0){ //Luma
                    dc_index = 0;
                    ac_index = 1;
                } else { //Chroma
                    dc_index = 2;
                    ac_index = 3;
                }

                // DC coeff.
                diff = mcu_zz[0] - DC[k];
                DC[k] = mcu_zz[0];

                if (diff != 0)
                {
                    get_magnitude(diff, mag);

                    tjei_write_bits(enc, &bitstack, &location,
                                    enc->ehuffsize[dc_index][mag[1]], 
                                    enc->ehuffcode[dc_index][mag[1]]);

                    tjei_write_bits(enc, &bitstack, &location, mag[1], mag[0]);
                }
                else
                {
                    tjei_write_bits(enc, &bitstack, &location,
                                    enc->ehuffsize[dc_index][0], 
                                    enc->ehuffcode[dc_index][0]);
                }

                // AC coeffs.
                zero_i = 0;
                for (i = 63; i > 0; i--)
                {
                    if (mcu_zz[i] != 0)
                    {
                        zero_i = i;
                        break;
                    }
                }

                for (i = 1; i <= zero_i; i++)
                {
                    zero_count = 0;
                    for (;mcu_zz[i] == 0;)
                    {
                        zero_count++;
                        i++;
                        if (zero_count == 16)
                        {
                            tjei_write_bits(enc, &bitstack, &location, 
                            enc->ehuffsize[ac_index][0xF0], 
                            enc->ehuffcode[ac_index][0xF0]);
                            zero_count = 0;
                        }
                    }

                    get_magnitude(mcu_zz[i], mag);

                    ac_byte = 0;
                    ((byte_nibble *)&ac_byte)->zeroes = zero_count;
                    ((byte_nibble *)&ac_byte)->category = mag[1];

                    assert(zero_count < 0x10);
                    assert(mag[1] <= 10);

                    assert(enc->ehuffsize[ac_index][ac_byte] != 0);

                    tjei_write_bits(enc, &bitstack, &location, 
                    enc->ehuffsize[ac_index][ac_byte], 
                    enc->ehuffcode[ac_index][ac_byte]);

                    tjei_write_bits(enc, &bitstack, &location, mag[1], mag[0]);
                }

                if (zero_i != 63)
                {
                    tjei_write_bits(enc, &bitstack, &location, 
                    enc->ehuffsize[ac_index][0], 
                    enc->ehuffcode[ac_index][0]);
                }
            }
        }
    }
}

void jpeg_write_to_file(jpeg_encoder_t enc, const char *filename)
{
    FILE *out_file = fopen(filename, "wb");

    TJEJPEGHeader header;
    char jfif_mark[5] = "JFIF";
    char comment_str[] = "Created by Tiny JPEG Encoder";

    // JFIF header.
    header.SOI = tjei_be_word(0xffd8); // Sequential DCT
    header.APP0 = tjei_be_word(0xffe0);
    uint16_t jfif_len = sizeof(TJEJPEGHeader) - 4 /*SOI & APP0 markers*/;
    header.jfif_len = tjei_be_word(jfif_len);
    memcpy(header.jfif_id, jfif_mark, 5);
    header.version = tjei_be_word(0x0102);
    header.units = 1;
    header.x_density = tjei_be_word(0x0060);
    header.y_density = tjei_be_word(0x0060);
    header.x_thumb = 0;
    header.y_thumb = 0;
    fwrite(&header, sizeof(TJEJPEGHeader), 1, out_file);

    // Comment
    TJEJPEGComment com;
    com.com = tjei_be_word(0xfffe);
    com.com_len = tjei_be_word(2 + sizeof(comment_str) - 1);
    memcpy(com.com_str, comment_str, sizeof(comment_str) - 1);
    fwrite(&com, sizeof(TJEJPEGComment), 1, out_file);

    // Write Luma Q Table
    uint16_t DQT = tjei_be_word(0xffdb);
    uint8_t q_table[64];
    fwrite(&DQT, sizeof(uint16_t), 1, out_file);
    uint16_t len = tjei_be_word(67); // 2(len) + 1(id) + 64(matrix) = 67 = 0x43
    fwrite(&len, sizeof(uint16_t), 1, out_file);
    uint8_t precision_and_id = 0; // 0x0000 8 bits | 0x00id
    fwrite(&precision_and_id, sizeof(uint8_t), 1, out_file);
    for (int i = 0; i < 64; i++)
        q_table[i] = enc->q_table[0][i];
    fwrite(q_table, sizeof(uint8_t), 64 * sizeof(uint8_t), out_file);

    // Write Chroma Q Table
    fwrite(&DQT, sizeof(uint16_t), 1, out_file);
    fwrite(&len, sizeof(uint16_t), 1, out_file);
    precision_and_id = 1; // 0x0000 8 bits | 0x01id
    fwrite(&precision_and_id, sizeof(uint8_t), 1, out_file);
    for (int i = 0; i < 64; i++)
        q_table[i] = enc->q_table[1][i];
    fwrite(q_table, sizeof(uint8_t), 64 * sizeof(uint8_t), out_file);

    // WRITE FRAME
    TJEFrameHeader frame_header;
    frame_header.SOF = tjei_be_word(0xffc0);
    frame_header.len = tjei_be_word(17);
    frame_header.precision = 8;
    frame_header.width = tjei_be_word((uint16_t)enc->width);
    frame_header.height = tjei_be_word((uint16_t)enc->height);
    frame_header.num_components = 3;
    uint8_t tables[3] = {0, 1, 1};
    for (int i = 0; i < 3; ++i)
    {
        TJEComponentSpec spec;
        spec.component_id = (uint8_t)(i + 1); // No particular reason. Just 1, 2, 3.
        spec.sampling_factors = (uint8_t)17;
        spec.qt = tables[i];

        frame_header.component_spec[i] = spec;
    }
    fwrite(&frame_header, sizeof(TJEFrameHeader), 1, out_file);

    // WRITE HUFFMAN TABLES
    write_DHT(out_file, enc->ht_bits[0], enc->ht_vals[0], 0, 0);
    write_DHT(out_file, enc->ht_bits[1], enc->ht_vals[1], 1, 0);

    write_DHT(out_file, enc->ht_bits[2], enc->ht_vals[2], 0, 1);
    write_DHT(out_file, enc->ht_bits[3], enc->ht_vals[3], 1, 1);

    // WRITE SCAN HEADER
    TJEScanHeader scan_header;
    scan_header.SOS = tjei_be_word(0xffda);
    scan_header.len = tjei_be_word((uint16_t)(6 + (sizeof(TJEFrameComponentSpec) * 3)));
    scan_header.num_components = 3;

    tables[0] = 0;
    tables[1] = 0x11;
    tables[2] = 0x11;
    for (int i = 0; i < 3; ++i)
    {
        TJEFrameComponentSpec cs;
        // Must be equal to component_id from frame header above.
        cs.component_id = (uint8_t)(i + 1);
        cs.dc_ac = (uint8_t)tables[i];

        scan_header.component_spec[i] = cs;
    }
    scan_header.first = 0;
    scan_header.last = 63;
    scan_header.ah_al = 0;

    fwrite(&scan_header, sizeof(TJEScanHeader), 1, out_file);

    fwrite(enc->result->data, sizeof(uint8_t), enc->result->size, out_file);

    uint16_t EOI = tjei_be_word(0xffd9);
    fwrite(&EOI, sizeof(uint16_t), 1, out_file);
    fflush(out_file);
    fclose(out_file);
    return;
}