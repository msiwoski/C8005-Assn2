#pragma once
#include <stdatomic.h>
#include <stddef.h>

typedef struct
{
    atomic_size_t head;
    atomic_size_t tail;
    atomic_int reader_guard; // Makes sure that the reader doesn't read partially-written data if it catches up to writers

    void* mem; // Should probably be a static fixed-size buffer since it will never be freed
    size_t size;
    size_t elem_size;
} ring_buffer_t;

/**
 * Tries to add an element to the buffer. This will block if the buffer is full.
 *
 * @param buf       The buffer to initialise.
 * @param mem       The memory to use as a ciruclar buffer.
 * @param size      The number of elem_size elements that the buffer can hold.
 * @param elem_size The size of each element in the buffer.
 */
void ring_buffer_init(ring_buffer_t* buf, void* mem, size_t size, size_t elem_size);

/**
 * Tries to add an element to the buffer. This will block if the buffer is full.
 *
 * @param buf  The buffer to which to add the item.
 * @param item A pointer to the item (which will be copied by value) to add to the buffer.
 */
void ring_buffer_put(ring_buffer_t* buf, void* item);

/**
 * Retrieves the next item from the ring buffer. Blocks if there are no items available.
 *
 * @param buf The buffer from which to retrieve the item.
 * @param out Pointer to a variable that will hold the result. Must be >= buf->elem_size.
 */
void ring_buffer_get(ring_buffer_t* buf, void* out);