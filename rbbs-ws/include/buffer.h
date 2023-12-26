/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdlib.h>

typedef struct {
    int* buffer;
    size_t size;
    size_t start;
    size_t end;
    int is_full;
} RingBuffer;

RingBuffer* create_ring_buffer(size_t size);
void destroy_ring_buffer(RingBuffer* ring_buffer);

int is_ring_buffer_empty(const RingBuffer* ring_buffer);
int is_ring_buffer_full(const RingBuffer* ring_buffer);
size_t ring_buffer_size(const RingBuffer* ring_buffer);

int enqueue(RingBuffer* ring_buffer, int value);
int dequeue(RingBuffer* ring_buffer, int* value);

#endif  // RING_BUFFER_H
