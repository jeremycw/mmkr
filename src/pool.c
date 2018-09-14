#include <stdlib.h>
#include <pthread/pthread.h>
#include <string.h>

#include "pool.h"

void pool_new(pool_t* pool, size_t size) {
    pool->current.generation = 0;
    pool->current.index = 0;
    pool->writers = 0;
    pool->size = size;
    pool->buffer = malloc(pool->size);
    pool->write_queue.head = 0;
    pool->write_queue.tail = 0;
    pool->write_queue.size = 0;
    pool->write_queue.capacity = 32;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
}

write_t* write_queue_enqueue(write_queue_t* write_queue, size_t size) {
    write_queue->queue[write_queue->tail].done = 0;
    write_queue->queue[write_queue->head].size = size;
    write_t* w = &write_queue->queue[write_queue->tail];
    write_queue->tail = (write_queue->tail + 1) % write_queue->capacity;
    return w;
}

gen_index_t increment_index(gen_index_t gi, size_t capacity, size_t increment) {
    gen_index_t new;
    if (gi.index + increment >= capacity) {
        new.index = 0;
        new.generation = gi.generation + 1;
    }
    new.index = gi.index + increment;
    return new;
}

int cmp_index(gen_index_t a, gen_index_t b) {
    if (a.generation > b.generation || (a.index > b.index && a.generation == b.generation)) {
        return 1;
    } else if (a.index == b.index && a.generation == b.generation) {
        return 0;
    } else {
        return -1;
    }
}

int write_queue_process(pool_t* pool) {
    gen_index_t initial = pool->safe;
    write_queue_t* write_queue = &pool->write_queue;
    while (write_queue->queue[write_queue->head].done && write_queue->head <= write_queue->tail) {
        size_t size = write_queue->queue[write_queue->head].size;
        pool->safe = increment_index(pool->safe, pool->size, size);
        write_queue->head++;
    }
    return cmp_index(initial, pool->safe);
}

void pool_write(pool_t* pool, void* src, size_t size) {
    pthread_mutex_lock(&pool->mutex);
    void* addr = &pool->buffer[pool->current.index];
    pool->current = increment_index(pool->current, pool->size, size);
    pool->writers++;
    write_t* write = write_queue_enqueue(&pool->write_queue, size);
    pthread_mutex_unlock(&pool->mutex);
    memcpy(addr, src, size);
    pthread_mutex_lock(&pool->mutex);
    pool->writers--;
    write->done = 1;
    write_queue_process(pool);
    if (pool->writers == 0) pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
}

size_t copy(pool_t* pool, pool_reader_t* reader, void* dst, size_t safe_bytes, size_t desired_bytes) {
    size_t bytes = safe_bytes > desired_bytes ? desired_bytes : safe_bytes;
    memcpy(dst, &pool->buffer[reader->index], bytes);
    *reader = increment_index(*reader, pool->size, bytes);
    return bytes;
}

size_t pool_read(pool_t* pool, pool_reader_t* pool_reader, void* dst, size_t size) {
    pthread_mutex_lock(&pool->mutex);
    if (cmp_index(*pool_reader, pool->safe) != -1) {
        pthread_cond_wait(&pool->cond, &pool->mutex);
    }
    gen_index_t safe = pool->safe;
    pthread_mutex_unlock(&pool->mutex);
    if (safe.index > pool_reader->index) {
        size_t diff = safe.index - pool_reader->index;
        return copy(pool, pool_reader, dst, diff, size);
    } else {
        size_t first_bytes = pool->size - pool_reader->index;
        size_t actual_bytes = copy(pool, pool_reader, dst, first_bytes, size);
        if (actual_bytes < size) return actual_bytes;
        size_t second_bytes = safe.index;
        actual_bytes += copy(pool, pool_reader, dst + first_bytes, second_bytes, size);
        return actual_bytes;
    }
}

void pool_swap(pool_t* pool, void** out, int* bytes, unsigned int flags) {
    pthread_mutex_lock(&pool->mutex);
    if (pool->writers > 0 || (flags & POOL_WAIT_FOR_DATA && pool->current.index == 0)) {
        pthread_cond_wait(&pool->cond, &pool->mutex);
    }
    *bytes = pool->current.index;
    *out = pool->buffer;
    pool->buffer = malloc(pool->size);
    pool->current.index = 0;
    pthread_mutex_unlock(&pool->mutex);
}