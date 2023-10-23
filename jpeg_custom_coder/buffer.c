#include "buffer.h"

buffer_t buffer_alloc(size_t size)
{
    buffer_t buf = (buffer_t)malloc(sizeof(struct resizable_buffer));

    buf->size = size;
    buf->data = NULL;

    if (buf->size != 0)
    {
        buf->data = (uint8_t *)malloc(buf->size);
        memset(buf->data, 0, buf->size);
    }

    return buf;
}

void buffer_free(buffer_t buf)
{
    assert(buf);

    if (buf->data != NULL)
        free(buf->data);

    free(buf);
}

void buffer_append(buffer_t buf, uint8_t *data, size_t size)
{
    assert(buf != NULL);
    assert(data != NULL);

    if (buf->size == 0)
        buf->data = (uint8_t *)malloc(size);
    else
        buf->data = (uint8_t *)realloc(buf->data, buf->size + size);

    memcpy(buf->data + buf->size, data, size);

    buf->size += size;
}

buffer_t buffer_deep_copy(buffer_t buf)
{
    assert(buf);

    buffer_t new = buffer_alloc(buf->size);

    new->data = (uint8_t *)malloc(buf->size);
    memcpy(new->data, buf->data, buf->size);
    new->size = buf->size;

    return new;
}

buffer_t buffer_shallow_copy(buffer_t buf)
{
    assert(buf);

    buffer_t new = buffer_alloc(buf->size);

    new->data = buf->data;
    new->size = buf->size;

    return new;
}

void buffer_reset(buffer_t buf, size_t new_size){
    if (buf->size != 0)
        free(buf->data);

    buf->size = new_size;
    if (buf->size != 0)
    {
        buf->data = (uint8_t *)malloc(buf->size);
        memset(buf->data, 0, buf->size);
    }
}

void buffer_resize(buffer_t buf, size_t new_size){
    if (buf->size == 0){
        buf->data = (uint8_t*)malloc(new_size);
        buf->size = new_size;
        return;
    }

    buf->data = (uint8_t*)realloc(buf->data, new_size);
    if (new_size > buf->size){
        memset(buf->data + buf->size, 0, new_size - buf->size);
    }

    buf->size = new_size;
}

void buffer_printhex(buffer_t buf)
{
    assert(buf);

    if (buf->data == NULL)
    {
        printf("null\n");
        return;
    }

    for (size_t i = 0; i < buf->size; i++)
    {
        printf("0x%X ", buf->data[i] & 0xFF);
    }
    printf("\n");
}
