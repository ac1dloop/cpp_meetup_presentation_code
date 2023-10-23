#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/**
 * @brief Resizable buffer API
 */
struct resizable_buffer
{
    uint8_t *data;
    size_t size;
};

typedef struct resizable_buffer *buffer_t;

/**
 * @brief Create empty 0 init buffer
 *
 * @param size
 * @return buffer_t
 */
buffer_t buffer_alloc(size_t size);

/**
 * @brief Delete buffer
 *
 * @param buf
 */
void buffer_free(buffer_t buf);

/**
 * @brief Append data to buffer
 *
 * @param buf
 * @param data
 * @param size
 */
void buffer_append(buffer_t buf, uint8_t *data, size_t size);

/**
 * @brief Create copy of buffer and data
 *
 * @param buf
 * @return buffer_t
 */
buffer_t buffer_deep_copy(buffer_t buf);

/**
 * @brief Create buffer that points to same data as 'buf'
 *
 * @param buf
 * @return buffer_t
 */
buffer_t buffer_shallow_copy(buffer_t buf);

void buffer_reset(buffer_t buf, size_t new_size);

void buffer_resize(buffer_t buf, size_t new_size);

void buffer_printhex(buffer_t buf);

#endif // BUFFER_H