#include <stdatomic.h>
#include <string.h>

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t* buf, void* mem, size_t size, size_t elem_size)
{
    buf->mem = mem;
    buf->size = size;
    buf->elem_size = elem_size;
    buf->head = 0;
    buf->tail = 0;
    buf->reader_guard = 0;
}

void ring_buffer_put(ring_buffer_t* buf, void* item)
{
    int wrote = 0;
    do
    {
        size_t head = atomic_load(&buf->head);
        size_t tail = atomic_load(&buf->tail);
        size_t diff = tail - head; // This will eventually wrap around, but we're unlikely to hit that case ¯\_(ツ)_/¯

        if (diff < buf->size &&
            atomic_compare_exchange_strong(&buf->tail, &tail, tail + 1))
        {
            // There was space for another element in the buffer and we successfully added to tail,
            // so we can safely write to the buffer... probably
            size_t pos = (tail % buf->size) * buf->elem_size;
            unsigned char* dest = (unsigned char*)buf->mem + pos;
            memcpy(dest, item, buf->elem_size);

            wrote = 1;
            atomic_store_explicit(&buf->reader_guard, 1, memory_order_release);
        }
    } while (!wrote);
}

void ring_buffer_get(ring_buffer_t* buf, void* out)
{
    int read = 0;
    do
    {
        // TODO: Do we actually need memory_order_acquire for head? Only one thread ever modifies it (or should, at least)
        size_t head = atomic_load(&buf->head);
        size_t tail = atomic_load(&buf->tail);

        // There's no data to read - set the reader guard to 0 so that we'll block until some other thread writes
        if (head == tail)
        {
            atomic_store(&buf->reader_guard, 0); // Not sure if this needs to be sequentially consistent
            break;
        }
        else
        {
            // Also not sure if this needs to be sequentially consistent (provided that the store to reader_guard is
            int reader_guard = atomic_load_explicit(&buf->reader_guard, memory_order_acquire);
            if (reader_guard)
            {
                unsigned char* mem = (unsigned char*)buf->mem;
                size_t pos = (head % buf->size) * buf->elem_size;
                memcpy(out, mem + pos, buf->elem_size);

                atomic_store_explicit(&buf->head, head + 1, memory_order_release);
                read = 1;
            }
        }
    } while(!read);
}