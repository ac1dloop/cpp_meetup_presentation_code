#include "ppmm.h"
#include "timer.h"
#include "jpeg.h"

int test_encode(int argc, char **argv)
{
    char *in_filename;
    char *out_filename;
    PPMImg *img = NULL;
    jpeg_encoder_t enc = NULL;

    if (argc < 2)
        in_filename = "/home/a/win/Pictures/borabora_1.ppm";
    else
        in_filename = argv[1];

    if (argc < 3)
        out_filename = "result.jpg";
    else
        out_filename = argv[2];

    img = PPMImg_from_file(in_filename);

    if (img == NULL)
    {
        printf("Error: img is NULL\n");
        return -1;
    }

    enc = jpeg_alloc();

    enc->compression_lvl = 3;

    PPMImg_print_header(img);

    Timer_t timer;
    timer_start(&timer);
    jpeg_encode_data(enc, img->width, img->height, (const uint8_t*)img->data);

    printf("Encoded in: %ldms\n", timer_delta_ms(&timer));
    printf("Compressed payload: %zu bytes\n", enc->result->size);

    jpeg_write_to_file(enc, out_filename);

    jpeg_free(enc);
    PPMImg_free(img);
    return 0;
}

int main(int argc, char **argv)
{
    return test_encode(argc, argv);

    return 0;
}