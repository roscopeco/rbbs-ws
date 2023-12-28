/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#include "ring_buffer.h"

RingBuffer* create_ring_buffer(size_t size) {
    RingBuffer* ring_buffer = (RingBuffer*)malloc(sizeof(RingBuffer));
    if (ring_buffer == NULL) {
        // Handle memory allocation failure
        return NULL;
    }

    ring_buffer->buffer = (unsigned char*)malloc(size * sizeof(unsigned char));
    if (ring_buffer->buffer == NULL) {
        // Handle memory allocation failure
        free(ring_buffer);
        return NULL;
    }

    ring_buffer->size = size;
    ring_buffer->start = 0;
    ring_buffer->end = 0;
    ring_buffer->is_full = 0;

    return ring_buffer;
}

void destroy_ring_buffer(RingBuffer* ring_buffer) {
    if (ring_buffer != NULL) {
        free(ring_buffer->buffer);
        free(ring_buffer);
    }
}

int is_ring_buffer_empty(const RingBuffer* ring_buffer) {
    return (!ring_buffer->is_full && (ring_buffer->start == ring_buffer->end));
}

int is_ring_buffer_full(const RingBuffer* ring_buffer) {
    return ring_buffer->is_full;
}

size_t ring_buffer_size(const RingBuffer* ring_buffer) {
    if (ring_buffer->is_full) {
        return ring_buffer->size;
    } else if (ring_buffer->end >= ring_buffer->start) {
        return ring_buffer->end - ring_buffer->start;
    } else {
        return ring_buffer->size - (ring_buffer->start - ring_buffer->end);
    }
}

int enqueue(RingBuffer* ring_buffer, int value) {
    if (is_ring_buffer_full(ring_buffer)) {
        // Cannot enqueue into a full ring buffer
        return -1;
    }

    ring_buffer->buffer[ring_buffer->end] = value;
    ring_buffer->end = (ring_buffer->end + 1) % ring_buffer->size;

    if (ring_buffer->end == ring_buffer->start) {
        ring_buffer->is_full = 1;
    }

    return 0;
}

int dequeue(RingBuffer* ring_buffer, int* value) {
    if (is_ring_buffer_empty(ring_buffer)) {
        // Cannot dequeue from an empty ring buffer
        return -1;
    }

    *value = ring_buffer->buffer[ring_buffer->start];
    ring_buffer->start = (ring_buffer->start + 1) % ring_buffer->size;
    ring_buffer->is_full = 0;

    return 0;
}

