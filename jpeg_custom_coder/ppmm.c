#include "ppmm.h"

// create empty
PPMImg *PPMImg_alloc()
{
    PPMImg *img = (PPMImg *)malloc(sizeof(PPMImg));

    img->data = NULL;

    return img;
}

// release memory
void PPMImg_free(PPMImg *img)
{
    if (img->data != NULL)
        free(img->data);

    free(img);
}

// print header
void PPMImg_print_header(PPMImg *img)
{
    printf("P%d %zux%zu %d\n", img->type == 0 ? 3 : 6, img->width, img->height, img->max_val);
}

// access pixel data
RGBPixel *PPMImg_pixel_at(PPMImg *img, size_t i, size_t j)
{
    return &img->data[i * img->width + j];
}

// create from buffer of data
PPMImg *PPMImg_from_buf(const char *buf, size_t buf_size)
{

    PPMImg *img = PPMImg_alloc(); // result
    size_t i = 0;                 //'buf' iterator

    char magic_number[2];

    magic_number[0] = buf[i];
    magic_number[1] = buf[i + 1];

    if (magic_number[0] != 'P')
    {
        printf("wrong mn: %c\n", magic_number[0]);
    }

    if (magic_number[1] == '3')
    {
        img->type = 0;
    }
    else
    {
        img->type = 1;
    }

    // ascii and binary formats have different comment types
    if (img->type != 1)
    {
        printf("Cant work with ascii\n");
        PPMImg_free(img);
        return NULL;
    }

    // move to next line
    i += 2;
    for (; isspace(buf[i]); i++)
        ;

    // TODO: skip multiple comments?
    // skip comment
    if (buf[i] == '#')
    {
        for (; buf[i] != '\n'; i++)
            ;
    }

    for (; isspace(buf[i]); i++)
        ;

    // get width
    char *tmp = (char *)calloc(MAX_STR_BUF, sizeof(char));
    for (size_t j = 0; buf[i] != ' '; j++, i++)
    {
        tmp[j] = buf[i];
    }
    i++;

    img->width = atoi(tmp);

    // get height
    memset(tmp, 0, MAX_STR_BUF);
    for (size_t j = 0; !isspace(buf[i]); j++, i++)
    {
        tmp[j] = buf[i];
    }

    img->height = atoi(tmp);

    // move to next line
    for (; isspace(buf[i]); i++)
        ;

    // get max pixel value
    memset(tmp, 0, MAX_STR_BUF);
    for (size_t j = 0; !isspace(buf[i]); j++, i++)
    {
        tmp[j] = buf[i];
    }

    img->max_val = atoi(tmp);

    // move to next line
    for (; isspace(buf[i]); i++)
        ;

    img->data = (RGBPixel *)malloc(sizeof(RGBPixel) * img->width * img->height);

    for (size_t j = 0; j < img->width * img->height; j++)
    {
        img->data[j].red = buf[i++];
        img->data[j].green = buf[i++];
        img->data[j].blue = buf[i++];
    }

    free(tmp);
    return img;
}

buffer_t PPMImg_to_buf(PPMImg *img)
{
    // total header size
    // P6\n -- 3 bytes
    //[width] [height]\n -- unknown
    // 255\n -- 4 bytes
    // data\n -- width * height * 3 + 1
    assert(img != NULL);

    size_t i = 0;   // result buffer iterator
    char tmp[128];  // temporary string for sprintf

    memset(tmp, 0, 128);

    sprintf(tmp, "%zu %zu\n", img->width, img->height);

    // magic num + width height + max_val per pixel + R,G,B channels 1 byte each + '\n'
    buffer_t ret = buffer_alloc(3 + strlen(tmp) + 4 + img->width * img->height * 3 + 1); 

    memcpy(ret->data, "P6\n", 3);
    i += 3;

    memcpy(ret->data + i, tmp, strlen(tmp));
    i += strlen(tmp);

    memcpy(ret->data + i, "255\n", 4);
    i += 4;

    // in case of padding struct
    assert(img->width * img->height * sizeof(RGBPixel) < ret->size - i);

    memcpy(ret->data + i, img->data, img->width * img->height * sizeof(RGBPixel));
    i += img->width * img->height * sizeof(RGBPixel);

    ret->data[i] = '\n';

    return ret;
}

// create from file
PPMImg *PPMImg_from_file(const char *filename)
{

    FILE *in_file = fopen(filename, "rb");

    char *buf = NULL;

    if (in_file == NULL)
    {
        printf("cannot open file\n");
        return NULL;
    }

    fseek(in_file, 0, SEEK_END);

    size_t buf_sz = ftell(in_file);

    buf = (char *)malloc(buf_sz * sizeof(char));

    fseek(in_file, 0, SEEK_SET);

    size_t to_read = 0;
    for (; to_read < buf_sz;)
    {
        to_read += fread(buf + to_read, sizeof(char), buf_sz, in_file);
    }

    fclose(in_file);

    PPMImg *img = PPMImg_from_buf(buf, buf_sz);

    if (img == NULL)
    {
        printf("failed to read img\n");
        return NULL;
    }

    return img;
}

void PPMImg_to_file(PPMImg *img, const char *filename)
{

    FILE *out_file = fopen(filename, "w+b");

    if (out_file == NULL)
    {
        printf("failed to open file\n");
    }

    buffer_t to_write = PPMImg_to_buf(img);

    size_t written = fwrite(to_write->data, sizeof(char), to_write->size, out_file);

    if (written < to_write->size)
    {
        printf("Failed to write to file\n");
    }

    buffer_free(to_write);
    fflush(out_file);
    fclose(out_file);
}